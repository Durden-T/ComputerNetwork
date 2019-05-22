#include <stdio.h>
#include <string.h>
#include<stdbool.h>

#include "protocol.h"
#include "datalink.h"

#define MAX_SEQ 15
#define DATA_TIMER  1500
#define ACK_TIMER 300


struct FRAME
{
	unsigned char kind; //FRAME_DATA，FRAME_ACK
	unsigned char ack;
	unsigned char seq;
	unsigned char data[PKT_LEN];
	unsigned int  padding;
};

static bool phl_ready = false;

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


static void my_send_frame(unsigned char fk, unsigned char frame_nr, unsigned char frame_ecpected, unsigned char buffer[MAX_SEQ + 1][PKT_LEN])
{
	//Constructand send a data, ack frame.
	struct FRAME s; // scratch variable
	s.kind = fk;// kind == data, ack
	s.ack = (frame_ecpected + MAX_SEQ) % (MAX_SEQ + 1);

	if (fk == FRAME_DATA)
	{
		s.seq = frame_nr; // only meaningful for data frames
		memcpy(s.data, buffer[frame_nr], PKT_LEN);

		dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short*)s.data);
		put_frame((unsigned char*)& s, 3 + PKT_LEN);//transmit the frame
		start_timer(frame_nr, DATA_TIMER);
	}
	else if (fk == FRAME_ACK)
	{
		dbg_frame("Send ACK %d\n", s.ack);
		put_frame((unsigned char*)& s, 2);
	}
	stop_ack_timer();// no need for separate ack frame
}

int main(int argc, char** argv)
{
	int event, arg;
	struct FRAME f;
	int len = 0;

	unsigned char ack_expected = 0; //lower edge of sender’s window
	unsigned char frame_nr = 0;//frame_nr, used for outbound stream
	unsigned char frame_expected = 0;//next ack_expected on the inbound stream

	unsigned char nbuffered = 0;//how many output buffers currently used

	unsigned char buffer[MAX_SEQ + 1][PKT_LEN]; // buffers for the outbound stream

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
			get_packet(buffer[frame_nr]);// fetch new packet
			my_send_frame(FRAME_DATA, frame_nr, frame_expected, buffer);//transmit the frame
			frame_nr = (frame_nr + 1) % (MAX_SEQ + 1);// advance upper window edge
			break;

		case PHYSICAL_LAYER_READY://physical layer is ready to send

			phl_ready = true;
			break;

		case FRAME_RECEIVED://a data or control frame has arrived

			len = recv_frame((unsigned char*)& f, sizeof f);
			if (len < 5 || crc32((unsigned char*)& f, len) != 0)//check whether this frame is legal and expected
			{
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				break;
			}

			if (f.kind == FRAME_DATA)
			{
				dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short*)f.data);
				if (f.seq == frame_expected)
				{
					put_packet(f.data, len - 7);// pass packet to network layer
					frame_expected = (frame_expected + 1) % (MAX_SEQ + 1); // advance lower edge of receiver’s window
					start_ack_timer(ACK_TIMER);
				}
			}
			else if (f.kind == FRAME_ACK && between(ack_expected, (f.ack + 1) % (MAX_SEQ + 1), frame_nr))
				dbg_frame("Recv FRAME_ACK  %d\n", f.ack);

			//Ack n implies n − 1, n − 2, etc.Check for this.
			while (between(ack_expected, f.ack, frame_nr))
			{
				//Handle piggybacked ack.
				--nbuffered; // one frame fewer buffered
				stop_timer(ack_expected); //frame arrived intact; stop timer
				ack_expected = (ack_expected + 1) % (MAX_SEQ + 1); // contract sender’s window
			}
			break;

		case DATA_TIMEOUT://data timer timeout resend

			dbg_event("---- DATA %d timeout\n", arg);
			frame_nr = ack_expected; //start retransmitting here
			for (int i = 1; i <= nbuffered; ++i)
			{
				my_send_frame(FRAME_DATA, frame_nr, frame_expected, buffer);// resend frame
				frame_nr = (frame_nr + 1) % (MAX_SEQ + 1); // prepare to send the next one
			}
			break;

		case ACK_TIMEOUT://no frame to send,have to send ack frame

			dbg_event("---- ACK timeout\n");
			my_send_frame(FRAME_ACK, 0, frame_expected, buffer);
			break;
		}

		if (nbuffered < MAX_SEQ && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}