#include <stdio.h>
#include <string.h>
#include<stdbool.h>

#include "protocol.h"
#include "datalink.h"

#define MAX_SEQ 15
#define NR_BUFS ((MAX_SEQ+1)/2)
#define DATA_TIMER 3000
#define ACK_TIMER 300


struct FRAME
{
	unsigned char kind; //FRAME_DATA，FRAME_ACK，FRAME_NAK
	unsigned char ack;
	unsigned char seq;
	unsigned char data[PKT_LEN];
	unsigned int  padding;
};

static bool phl_ready = false;//physical layer is ready
static bool no_nak = true; // no nak has been sent yet

//Return true if a <= b < c circularly; false otherwise.
inline bool between(unsigned char a, unsigned char b, unsigned char c)
{
	return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}

static void put_frame(unsigned char* frame, int len)
{
	*(unsigned int*)(frame + len) = crc32(frame, len);
	send_frame(frame, len + 4);
	phl_ready = false;
}

static void my_send_frame(unsigned char fk, unsigned char frame_nr, unsigned char frame_ecpected, unsigned char buffer[NR_BUFS][PKT_LEN])
{
	//Constructand send a data, ack, or nak frame.
	struct FRAME s; // scratch variable
	s.kind = fk;// kind == data, ack, or nak
	s.ack = (frame_ecpected + MAX_SEQ) % (MAX_SEQ + 1);

	if (fk == FRAME_DATA)
	{
		s.seq = frame_nr; // only meaningful for data frames
		memcpy(s.data, buffer[frame_nr % NR_BUFS], PKT_LEN);

		dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short*)s.data);
		put_frame((unsigned char*)& s, 3 + PKT_LEN);//transmit the frame
		start_timer(frame_nr % NR_BUFS, DATA_TIMER);
	}
	else if (fk == FRAME_ACK)
	{
		dbg_frame("Send ACK %d\n", s.ack);
		put_frame((unsigned char*)& s, 2);
	}
	else
	{
		no_nak = false;// one nak per frame, please
		dbg_frame("Send NAK %d\n", s.ack);
		put_frame((unsigned char*)& s, 2);
	}
	stop_ack_timer();// no need for separate ack frame
}

int main(int argc, char** argv)
{
	int event, arg;//event and event arg
	struct FRAME f;//temp frame
	int len = 0;//length of receive frame

	unsigned char ack_expected = 0; //lower edge of sender’s window
	unsigned char frame_nr = 0;//next frame to send,upper edge of sender’s window + 1, number of next outgoing frame
	unsigned char frame_expected = 0;// lower edge of receiver’s window, next ack expected on the inbound stream
	unsigned char too_far = NR_BUFS;//upper edge of receiver’s window + 1

	unsigned char nbuffered = 0;//how many output buffers currently used

	static unsigned char out_buf[NR_BUFS][PKT_LEN]; // buffers for the outbound stream
	static unsigned char in_buf[NR_BUFS][PKT_LEN]; // buffers for the inbound stream
	static bool arrived[NR_BUFS]; // inbound bit map, static auto false

	protocol_init(argc, argv);
	lprintf("Designed by Jiang Yanjun, changed by Qu Chaoyue, build: " __DATE__"  "__TIME__"\n");
	disable_network_layer();

	for (;;)
	{
		event = wait_for_event(&arg);
		switch (event)//five possibilities: see event type above
		{
		case NETWORK_LAYER_READY://accept, save, and transmit a new frame

			++nbuffered;//expand the window
			get_packet(out_buf[frame_nr % NR_BUFS]);// fetch new packet
			my_send_frame(FRAME_DATA, frame_nr, frame_expected, out_buf);//transmit the frame
			frame_nr = (frame_nr + 1) % (MAX_SEQ + 1);// advance upper window edge
			break;

		case PHYSICAL_LAYER_READY://physical layer is ready to send

			phl_ready = true;
			break;

		case FRAME_RECEIVED://a data or control frame has arrived

			len = recv_frame((unsigned char*)& f, sizeof f);
			if (len < 5 || crc32((unsigned char*)& f, len) != 0)//check whether this frame is legal and expected
			{
				if (no_nak)
					my_send_frame(FRAME_NAK, 0, frame_expected, out_buf);//send nak
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				break;
			}

			if (f.kind == FRAME_DATA)
			{
				dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short*)f.data);
				if (f.seq != frame_expected && no_nak)//not in order
					my_send_frame(FRAME_NAK, 0, frame_expected, out_buf);
				else
					start_ack_timer(ACK_TIMER);

				if (between(frame_expected, f.seq, too_far) && !arrived[f.seq % NR_BUFS])
				{
					//Frames may be accepted in any order
					arrived[f.seq % NR_BUFS] = true;// mark buffer as full
					memcpy(in_buf[f.seq % NR_BUFS], f.data, len - 7);

					while (arrived[frame_expected % NR_BUFS])
					{
						//Pass frames and advance window.
						put_packet(in_buf[frame_expected % NR_BUFS], PKT_LEN);
						no_nak = true;
						arrived[frame_expected % NR_BUFS] = false;
						frame_expected = (frame_expected + 1) % (MAX_SEQ + 1); // advance lower edge of receiver’s window
						too_far = (too_far + 1) % (MAX_SEQ + 1);  // advance upper edge of receiver’s window
						start_ack_timer(ACK_TIMER); // to see if a separate ack is needed
					}
				}
			}
			else if (f.kind == FRAME_ACK && between(ack_expected, (f.ack + 1) % (MAX_SEQ + 1), frame_nr))
				dbg_frame("Recv FRAME_ACK  %d\n", f.ack);
			else if ((f.kind == FRAME_NAK) && between(ack_expected, (f.ack + 1) % (MAX_SEQ + 1), frame_nr))
			{
				dbg_frame("Recv FRAME_NAK %d\n", f.ack);
				my_send_frame(FRAME_DATA, (f.ack + 1) % (MAX_SEQ + 1), frame_expected, out_buf);
			}

			while (between(ack_expected, f.ack, frame_nr))
			{
				--nbuffered; //handle piggybacked ack
				stop_timer(ack_expected % NR_BUFS); //frame arrived intact,advance lower edge of sender’s window
				ack_expected = (ack_expected + 1) % (MAX_SEQ + 1);//advance lower edge of sender’s window
			}
			break;

		case DATA_TIMEOUT://data timer timeout resend

			dbg_event("---- DATA %d timeout\n", arg);
			my_send_frame(FRAME_DATA, ack_expected, frame_expected, out_buf);
			break;

		case ACK_TIMEOUT://no frame to send,have to send ack frame

			dbg_event("---- ACK timeout\n");
			my_send_frame(FRAME_ACK, 0, frame_expected, out_buf);
			break;
		}
		if (nbuffered < NR_BUFS && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}