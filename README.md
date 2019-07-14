# data-exfiltration
An encoding and a decoding tool for use in storing data in PNG files.

## File Index
* `encoder.c` contains the `main()` for the encoding program. The `encoder` takes in two arguments: the name of the PNG file that will hide the payload, and the name of the payload itself. It creates a file called `output.png` that contains the hidden payload. 
It's usage is
```./encoder <input PNG filepath> <payload filepath>```.

* `decoder.c` contains the `main()` for the decoding program. The `decoder` takes in two arguments: the name of the PNG file that is hiding a payload, and the name of the file that the payload will be dumped into. It creates a file with the name given that will contain the hidden payload. 
It's usage is
```./decoder <input PNG filepath> <output payload filepath>```

* `png.h` contains my own custom interface to the PNG file format. There are many like it, but this one is mine.

## What all can I exfiltrate with these programs?
Really anything. I tried it on some generic text, executables, and even another PNG file. Pretty much anything under the PNG maximum allowed chunk size of about 4 GB. I think that in my code it limits you to a payload file of 2 GB in size. Most operating systems won't let you allocate and fill 4 GB of physical memory in user space anyways but, hey, you can try. Besides, who is going to see a 4 GB PNG file and think, "Yeah that's normal"?

If you really need to store a ridiculous amount of data in a PNG file, you can! Just break your payload up into several files (each under the 2 GB limit) and run the `encoder` multiple times. Take the output PNG from the first run and use it as the input PNG for the second run, and so on.
