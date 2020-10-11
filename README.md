# Memsim
Attempts to emulate various memory paging designs.  
  
Project for an operating systems class.  
  
This code reads in a .trace file located in the same location as the executable. From the trace file, it reads a variety of read and write operations and their corresponding memory locations.  
Then, depending on the memory scheme used, checks how many memory hits/misses are accumulated over the course of reading the file.
Memory schemes available are random, least recently used, FIFO and VMS.
