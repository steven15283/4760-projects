  1 the commit.txt file is the commit logs from git.If those logs are insufficie    nt, here is my github link to the project:https://github.com/steven15283/476    0-projects.git
  2 to run project 4 just do ./oss
  3 
  4 my base quantum is 10000000 nanoseconds. my blocked time increment is 1000000 nanoseconds.my idle increment is 100000 nanoseconds. my scheduling overhead is 1000 nanoseconds.
  5 My max time between new processes is 500000000 nanoseconds. I used a pointer structure to imitate a vector for my queue.
  6 Hopefully i did this right but my queues does from rrqueue->termination, queue1->expired queue1 ->queue 2 ->expired queue 2 -> queue 3 ->expired queue3->termination
  7 
  8 rrqueue will complete the total time slice and will never move to expired queue because its a real time process queue.For the rest, if a process uses the full quantum it will move to expired and then if it
  9 uses the full quantum in the expired it will then move on to the next queue.    
