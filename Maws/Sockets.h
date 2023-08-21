#ifndef MAWS_SOCKETS
#define MAWS_SOCKETS

struct socket_pair
{
	int socket;
	struct maws_state *state;

	socket_pair(int psock, struct maws_state *pstate)
	 : socket(psock), state(pstate) {}
};
void* connection_handler(void *socket_desc);
void* prep_socket(void *state);

#endif