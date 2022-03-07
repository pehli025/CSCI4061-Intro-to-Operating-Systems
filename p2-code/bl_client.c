#include "blather.h"
// Globals for the two threads
simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;
client_t client_actual;
client_t *client = &client_actual;
pthread_t user_thread;
pthread_t background_thread;
// user thread{
//   repeat:
//     read input using simpio
//     when a line is ready
//     create a mesg_t with the line and write it to the to-server FIFO
//   until end of input
//   print "End of Input, Departing"
//   write a DEPARTED mesg_t into to-server
//   cancel the server thread
void *user_worker(void *arg){
  while(!simpio->end_of_input) {
    simpio_reset(simpio);
    iprintf(simpio, "");
    while(!simpio->line_ready && !simpio->end_of_input) {          // read until line is complete
      simpio_get_char(simpio);
    }
    mesg_t msg = {};
    msg.kind = BL_MESG;
    if(simpio->line_ready){\
      strcpy(msg.name,client->name);
      strcpy(msg.body,simpio->buf);
      write(client->to_server_fd,&msg,sizeof(mesg_t));
    } else {
		msg.kind = BL_DEPARTED;
		iprintf(simpio, "End of Input, Departing\n");
		strcpy(msg.name,client->name);
		write(client->to_server_fd,&msg,sizeof(mesg_t));
	}
  }
  pthread_cancel(background_thread); // kill the background thread
  return NULL;
}
// Worker thread to manage user input
// user thread{
//   repeat:
//     read input using simpio
//     when a line is ready
//     create a mesg_t with the line and write it to the to-server FIFO
//   until end of input
//   print "End of Input, Departing"
//   write a DEPARTED mesg_t into to-server
//   cancel the server thread
void *background_worker(void *arg){
  while(1){
    mesg_t msg;
    read(client->to_client_fd,&msg,sizeof(mesg_t));
     if (msg.kind == BL_JOINED) {
      iprintf(simpio, "-- %s JOINED --\n", msg.name);
    } else if (msg.kind == BL_DEPARTED) {
      iprintf(simpio, "-- %s DEPARTED --\n", msg.name);
    } else if (msg.kind == BL_MESG) {
      iprintf(simpio, "[%s] : %s\n", msg.name, msg.body);
    } else if (msg.kind == BL_SHUTDOWN) {
      iprintf(simpio, "!!! server is shutting down !!!\n");
      break;
    }
  }
  pthread_cancel(user_thread);
  return NULL;
}
// server thread{
//   repeat:
//     read a mesg_t from to-client FIFO
//     print appropriate response to terminal with simpio
//   until a SHUTDOWN mesg_t is read
//   cancel the user thread


// read name of server and name of user from command line args
// create to-server and to-client FIFOs
// write a join_t request to the server FIFO
// start a user thread to read input
// start a server thread to listen to the server
// wait for threads to return
// restore standard terminal output
int main(int argc, char *argv[]){
	if(argc < 3) {
		//return 0;
	}

	join_t join_request = {};
	char prompt[MAXNAME];

	strcpy(client->name,argv[2]);
	sprintf(client->to_server_fname, "toServer%d", getppid());
	sprintf(client->to_client_fname, "toClient%d",getppid());

	mkfifo(client->to_server_fname, S_IRUSR | S_IWUSR);
	mkfifo(client->to_client_fname, S_IRUSR | S_IWUSR);

	client->to_server_fd = open(client->to_server_fname,O_RDWR);
	client->to_client_fd = open(client->to_client_fname,O_RDWR);

	strcpy(join_request.name,client->name);
	strcpy(join_request.to_server_fname,client->to_server_fname); 
	strcpy(join_request.to_client_fname,client->to_client_fname);

	int join_fd_ = open(argv[1], O_WRONLY);
	write(join_fd_, &join_request, sizeof(join_t));

	snprintf(prompt, MAXNAME, "%s>> ", client->name);
	simpio_set_prompt(simpio, prompt);
	simpio_reset(simpio);
	simpio_noncanonical_terminal_mode(); 

	pthread_create(&user_thread,   NULL, user_worker,   NULL);
	pthread_create(&background_thread, NULL, background_worker, NULL);

	pthread_join(user_thread, NULL);
	pthread_join(background_thread, NULL);
	  
	simpio_reset_terminal_mode();

	return 0;
}
