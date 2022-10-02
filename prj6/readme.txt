commit.txt file is the commit logs from git.If those logs are insufficient, here is my github link to the project:https://github.com/steven15283/4760-projects.git
   
  To execute do ./oss 
  The oss loads a single structure into shared memory, a simulated clock. Then it enters an infinite loop where it will only terminate after 2 real life seconds. In       the  loop it will spawn processess at random intervals between 0 and 500000 ns. If it is not spawning a child then it is checking a message queue for messages from the   children. The message will be a request for memory or signalling normal termination.
  
  THeres 4 different outcomes when recieving a message from user. no page fault, pagefault(the page is not in memory but there is an empty frame that the page can         insterted into),
  pagefault(the page is not in memory but there is no empty frame and a page must be replaced using the algorithm), termination.
  
  The user begins by attaching to the simulated clock. It then enters a loop sending messages of random types to oss. As long as all requests are satisfied
  then user will continue looping until it randomly decides to terminate In which case it sends a message indicating it is terminating. If a request is denied then
  it will wait to be granted the resource by waiting to receive a message from oss.
  
  messaging rules:messages are always sent to specific processes using mtype,mtype is the process id of the message receiver in this case oss will have a pseudo-process   id of 20,therefore when sending a message to oss mtype will be 20,
  oss will only receive messages of mtype 20, children will use there simulated process ids, meaning they will only receive messages whose mtype = their simulated pid,     msg.sender is the message sender
