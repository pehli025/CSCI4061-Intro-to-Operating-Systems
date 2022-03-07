#include "blather.h"
#include <poll.h>

client_t *server_get_client(server_t *server, int idx){
    if (idx < server->n_clients){
        return &server->client[idx];
    }
    return server->client;
}
// Gets a pointer to the client_t struct at the given index. If the
// index is beyond n_clients, the behavior of the function is
// unspecified and may cause a program crash.

//concat fifo at end
void server_start(server_t *server, char *server_name, int perms){
    log_printf("BEGIN: server_start()\n");

    server->join_ready = 0;

    strcpy(server->server_name,server_name);
    remove(server_name);
    mkfifo(server_name,perms);
    server->join_fd = open(server_name, O_RDWR);

    log_printf("END: server_start()\n");             
}
// Initializes and starts the server with the given name. A join fifo
// called "server_name.fifo" should be created. Removes any existing
// file of that name prior to creation. Opens the FIFO and stores its
// file descriptor in join_fd.
//
// ADVANCED: create the log file "server_name.log" and write the
// initial empty who_t contents to its beginning. Ensure that the
// log_fd is position for appending to the end of the file. Create the
// POSIX semaphore "/server_name.sem" and initialize it to 1 to
// control access to the who_t portion of the log.
// 
// LOG Messages:
//  log_printf("BEGIN: server_start()\n");              // at beginning of function
// log_printf("END: server_start()\n");                // at end of function

void server_shutdown(server_t *server) {
  log_printf("BEGIN: server_shutdown()\n");
  close(server->join_fd);                             
  char fifo_path[MAXPATH];
  int n = server->n_clients;

  strncpy(fifo_path, server->server_name, MAXPATH);
  strncat(fifo_path, ".fifo", MAXPATH - strlen(".fifo"));
  mesg_t msg = {.kind = BL_SHUTDOWN};
  server_broadcast(server, &msg);
  int ret = 0;
  for (int i=0; i < n; i++) {
    ret = server_remove_client(server, i);
    check_fail(ret < 0, 0, "Server failed to remove client\n");
  }
  log_printf("END: server_shutdown()\n");
}
// Shut down the server. Close the join FIFO and unlink (remove) it so
// that no further clients can join. Send a BL_SHUTDOWN message to all
// clients and proceed to remove all clients in any order.
// 
// ADVANCED: Close the log file. Close the log semaphore and unlink
// it.
//
// LOG Messages:
// log_printf("BEGIN: server_shutdown()\n");           // at beginning of function
// log_printf("END: server_shutdown()\n");             // at end of function


int server_add_client(server_t *server, join_t *join){
    if(server->n_clients == MAXCLIENTS){
        return 1;
    }

    log_printf("BEGIN: server_add_client()\n");

    client_t newClient;
    strcpy(newClient.name,join->name);
    strcpy(newClient.to_server_fname,join->to_server_fname);
    strcpy(newClient.to_client_fname,join->to_client_fname);

    newClient.data_ready = 0;
    newClient.to_server_fd = open(newClient.to_server_fname, O_RDWR);
    newClient.to_client_fd = open(newClient.to_client_fname, O_RDWR);
    server -> client[server->n_clients] = newClient;
    server -> n_clients ++;

    log_printf("END: server_add_client()\n");
    return 0;
}
// Adds a client to the server according to the parameter join which
// should have fileds such as name filed in.  The client data is
// copied into the client[] array and file descriptors are opened for
// its to-server and to-client FIFOs. Initializes the data_ready field
// for the client to 0. Returns 0 on success and non-zero if the
// server as no space for clients (n_clients == MAXCLIENTS).
//
// LOG Messages:
// log_printf("BEGIN: server_add_client()\n");         // at beginning of function
// log_printf("END: server_add_client()\n");           // at end of function

int server_remove_client(server_t *server, int idx){
    client_t *tempClient = server_get_client(server,idx);
    
    if(remove(tempClient->to_client_fname) == -1){
        return 1;
    }
    if(remove(tempClient->to_server_fname) == -1){
        return 1;
    }
    int client = server->n_clients;
    for(int i=idx;i<client-1;i++){
        server->client[i] = server->client[i+1];
    }
    server->n_clients--;
    return 0;
}
// Remove the given client likely due to its having departed or
// disconnected. Close fifos associated with the client and remove
// them.  Shift the remaining clients to lower indices of the client[]
// preserving their order in the array; decreases n_clients. Returns 0
// on success, 1 on failure.


void server_broadcast(server_t *server, mesg_t *mesg){
    for(int i=0;i<server->n_clients;i++){
        write(server->client[i].to_client_fd,mesg,sizeof(mesg_t));
    }
}
// Send the given message to all clients connected to the server by
// writing it to the file descriptors associated with them.

void server_check_sources(server_t *server) {
  log_printf("BEGIN: server_check_sources()\n");

  int n = server->n_clients;
  struct pollfd pfds[n + 1];
  client_t *client;
  
  int i;
  for (i = 0; i <= n; i++) {
    pfds[i].events = POLLIN;
    pfds[i].revents = 0;
    if (i < n) {
      client = server_get_client(server, i);
      pfds[i].fd = client->to_server_fd;
    } else {
      pfds[i].fd = server->join_fd;
    }
  }

  log_printf("poll()'ing to check %d input sources\n", n + 1);
  int ret = poll(pfds, n + 1, -1);
  log_printf("poll() completed with return value %d\n", ret);
  if (ret < 0) {
    log_printf("poll() interrupted by a signal\n");
  } else {
    int ready = (pfds[n].revents & POLLIN);            // check join queue for data
    server->join_ready = ready;
    log_printf("join_ready = %d\n", ready);
    for (i = 0; i < n; i++) {                            // check clients for data
      int data_ready = pfds[i].revents & POLLIN;
      client = server_get_client(server, i);
      client->data_ready = data_ready;
      log_printf("client %d '%s' data_ready = %d\n", i, client->name, data_ready);       
    }  
  }
  log_printf("END: server_check_sources()\n");
}
// Checks all sources of data for the server to determine if any are
// ready for reading. Sets the servers join_ready flag and the
// data_ready flags of each of client if data is ready for them.
// Makes use of the poll() system call to efficiently determine which
// sources are ready.
// 
// NOTE: the poll() system call will return -1 if it is interrupted by
// the process receiving a signal. This is expected to initiate server
// shutdown and is handled by returning immediagely from this function.
// 
// LOG Messages:
// log_printf("BEGIN: server_check_sources()\n");             // at beginning of function
// log_printf("poll()'ing to check %d input sources\n",...);  // prior to poll() call
// log_printf("poll() completed with return value %d\n",...); // after poll() call
// log_printf("poll() interrupted by a signal\n");            // if poll interrupted by a signal
// log_printf("join_ready = %d\n",...);                       // whether join queue has data
// log_printf("client %d '%s' data_ready = %d\n",...)         // whether client has data ready
// log_printf("END: server_check_sources()\n");               // at end of function

int server_join_ready(server_t *server){
    return server->join_ready;
}
// Return the join_ready flag from the server which indicates whether
// a call to server_handle_join() is safe.

void server_handle_join(server_t *server){
    if(server_join_ready(server) == 1){
        log_printf("BEGIN: server_handle_join()\n");

        join_t join = {};
        mesg_t msg = {};
        msg.kind = 20;

        read(server->join_fd, &join, sizeof(join_t));
        log_printf("join request for new client '%s'\n", join.name);
        server_add_client(server, &join);
        strcpy(msg.name,join.name);
        strcat(msg.body,"-- ");
        strcat(msg.body,strcat(join.name," JOINED --"));
        server_broadcast(server,&msg);
        server->join_ready = 0;

        log_printf("END: server_handle_join()\n");                 // at end of function
    }
}
// Call this function only if server_join_ready() returns true. Read a
// join request and add the new client to the server. After finishing,
// set the servers join_ready flag to 0.
//
// LOG Messages:
// log_printf("BEGIN: server_handle_join()\n");               // at beginnning of function
// log_printf("join request for new client '%s'\n",...);      // reports name of new client
// log_printf("END: server_handle_join()\n");                 // at end of function

int server_client_ready(server_t *server, int idx) {
  int ready = server_get_client(server, idx) -> data_ready;
  dbg_printf("Client at index %d is ready: %d\n", idx, ready);
  return ready;
}
// Return the data_ready field of the given client which indicates
// whether the client has data ready to be read from it.

void server_handle_client(server_t *server, int idx){
  log_printf("BEGIN: server_handle_client()\n");

  mesg_t msg = {};
  client_t *client_ = server_get_client(server, idx);
  int to_server_fd = client_->to_server_fd;
  int nread = read(to_server_fd, &msg, sizeof(mesg_t));
  
  check_fail(nread < 0, 1, "Error reading from client FIFO\n");
  if (msg.kind == BL_MESG) {
    log_printf("client %d '%s' MESSAGE '%s'\n", idx, client_->name, msg.body);
    server_broadcast(server, &msg);
  } else if (msg.kind == BL_DEPARTED) {
    log_printf("client %d '%s' DEPARTED\n", idx, client_->name);
    server_remove_client(server, idx);
    server_broadcast(server, &msg);
  } else {
    dbg_printf("Unknown message kind received\n");
  }
  client_->data_ready = 0;

  log_printf("END: server_handle_client()\n");
}
// Process a message from the specified client. This function should
// only be called if server_client_ready() returns true. Read a
// message from to_server_fd and analyze the message kind. Departure
// and Message types should be broadcast to all other clients.  Ping
// responses should only change the last_contact_time below. Behavior
// for other message types is not specified. Clear the client's
// data_ready flag so it has value 0.
//
// ADVANCED: Update the last_contact_time of the client to the current
// server time_sec.
//
// LOG Messages:
// log_printf("BEGIN: server_handle_client()\n");           // at beginning of function
// log_printf("client %d '%s' DEPARTED\n",                  // indicates client departed
// log_printf("client %d '%s' MESSAGE '%s'\n",              // indicates client message
// log_printf("END: server_handle_client()\n");             // at end of function 
