#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum side {
  L = 0,
  R = 1,
};


/**
 * error is null unless something is wrong, then it is some explanation string
 */
struct args {
  int curDepth;
  int maxDepth;
  enum side lr;

  char *error;
};

struct args parseArgs(int argc, char *argv[]) {

}


struct PipedChild {
  // pid of the child
  int pid;
  // file descriptor to write to in order to send this child data
  int tx;
  // file descriptor to read in order to get data from this child
  int rx;
};

/**
 * this function: creates a pipe, sets parent stdout to pipe write, forks, returns id to parent. child: sets stdin to pipe read, uses execvp to become program
 * @param argv from exec manpage: The char *const argv[] argument is an array
 *             of pointers to nulintl-terminated strings that represent the
 *             argument list available to the new program. The first argument,
 *             by convention, should point to the filename associated with the
 *             file being executed. The array of pointers must be terminated by
 *             a null pointer.
 */

 struct PipedChild createPipedChild(char *program, char *argv[]) {
  // TODO

  // create two pipes, one to send, one to recieve 
  // (parent) [send, recieve]
  // inside: [ read, write ]
  int pipes[2][2];
  if (pipe(pipes[0]) == -1) exit(1);
  if (pipe(pipes[1]) == -1) exit(1);

  int pid = fork();
  
  if (pid == -1) exit(1);

  if (pid) {// is parent
    // close child ends of pipes
    close(pipes[0][0]);
    close(pipes[1][1]);
    struct PipedChild r = {
      pid, pipes[0][1], pipes[1][0]
    };
    return r;
    
  } else { // is child
    // close parent ends of pipes
    close(pipes[0][1]);
    close(pipes[1][0]);

    dup2(pipes[0][0], 0); // use pipe as stdin
    dup2(pipes[1][1], 1); // use pipe as stdout

    // go into whatever program
    execvp(program, argv);
  }

  exit(1); // normally unreachable?
}



/**
 * @brief wraps createWrappedChild
 * 
 * executes a right/left
 * by passing in the required numbers
 */
int doProgram(char *program, int num1, int num2) {

  char* argv[] = {program};
  struct PipedChild child = createPipedChild(program, argv);
  
  // // from assignment spec, strings through pipe are max length 10
  // // when writing to pipe, string should be null terminated.
  // char w_buffer[11];
  // w_buffer[10] = NULL;

  // // from manpage: writes max n characters into some string
  // snprintf(w_buffer, 10, "%d", num1);
  // if (write(child.tx, w_buffer, 11) == -1) exit(1);
  
  // // same for num2
  // snprintf(w_buffer, 10, "%d", num2);
  // if (write(child.tx, w_buffer, 11) == -1) exit(1);


  // also from assignment suggestion: use dprintf?
  dprintf(child.tx, "&%d", num1);
  dprintf(child.tx, "&%d", num2);

  // read 11 bytes because we know by spec that it is max 10
  char r_buffer[11];
  if (read(child.rx, r_buffer, 11) == -1) exit(1);
  return (atoi(r_buffer));

  // TODO: read from child and parse
  return -1;
}


int main(int argc, char *argv[]) {

  // three arguments from command line
  // num1 from scanf
  // spawn left child

  // decide what operation im doing: left or right
  // spawn right child, get result
  // printf result from right child
  // done


  printf("Hello World!\n");
  return 0;
}