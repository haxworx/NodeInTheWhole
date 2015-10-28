#include "bogomem.h"
#include "socket.h"

int main(void)
{
	
	init();

        Server(9000);

//	DumpData();

	return EXIT_SUCCESS;
}
