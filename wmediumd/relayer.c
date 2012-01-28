#include <zmq.h>

int main (void)
{
	// Prepare our context and sockets
	void *context = zmq_init (1);
	void *wmediumd = zmq_socket (context, ZMQ_REP);
	void *dispatch  = zmq_socket (context, ZMQ_REQ);
	zmq_bind (wmediumd, "ipc:///tmp/hwsim");
	zmq_connect (dispatch, "tcp://10.0.2.2:5555");

	while (1) {
		zmq_msg_t message;
		size_t more, more_size = sizeof(more);

		while (1) {
			/* Make sure we process all of the message */
			zmq_msg_init (&message);
			zmq_recv (wmediumd, &message, 0);
			zmq_getsockopt (wmediumd, ZMQ_RCVMORE, &more, &more_size);

			zmq_send (dispatch, &message, more? ZMQ_SNDMORE: 0);
			zmq_msg_close (&message);

			if (!more)
				break; /* We're done with this message, break */
		}

		/* Grab the one-part reply and relay that */
		zmq_msg_init(&message);
		zmq_recv(dispatch, &message, 0);
		zmq_send(wmediumd, &message, 0);
		zmq_msg_close(&message);
	}

	/* TODO: Detect C-x so we can get here */
	zmq_close (dispatch);
	zmq_close (wmediumd);
	zmq_term (context);
	return 0;
}
