Name: Golnoush Hassanzadeh
Student ID: 1549621
References: all references included in the sources code

Running Instruction:
after building using make command, please type:
./prodcon <numberofthreads> <fileid> < inputfile
or
./prodcon <numberofthreads> < inputfile
or 
./prodcon <numberofthreads>      ( to take the input from stdin)

Comments:
- When taking inputs from stdin, CTRL + D should be held twice if pressing the keys on the last line and only once if you go to a new line
- The inputs in each line must be in a correct format: T<int> or S<int>, both T and S must be in capital
- After producer added all works into the queue, i added sleep(1) for the consumers to catch up and read the remaining work from the queue. This may not be necessary but i added it just in case. I noticed it slightly decreased Transactions per second but the effect is not that significant.