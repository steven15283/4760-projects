 commit.txt file is the commit logs from git.If those logs are insufficient, here is my github link to the project:https://github.com/steven15283/4760-projects.git
  2 it includes verbose printing.
  3 
  4 To execute do ./oss
  5 if you want verbose printing do ./oss -v
  6 to look at the options do ./oss -h
  7 
  8 After getting verbose option from arguments, the program will load two structures into shared memory:clock and resource descriptor.
  9 it will run infinitely until 10 seconds has passed. the loop will spawn process betweeon 0-500ms.
 10 when its not spawning children then its checking the message queue for messages from the children.
 11 the message queue's job is to request for resource, release resource, signalling termination.
 12 If a deadlock is detected, the processes that are deadlocked are terminated. A deadlock is checked for every second.
 13 When a request isn't met, it sends a deny message and it is sent to the child.
 14 The child will then have to wait for another message to meet the request.
 15 
 16 As long as all the requests are met, the user program will continue looping until it randomly decides to terminate.
 17 This is between 0-250ms and it will send a termination message.
 18 A denied request will wait to be granted the resource by waiting for a message from oss.
~
