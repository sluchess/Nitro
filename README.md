# Nitro
Nitro allows for users to send and receive large files directly to and from one another.

Use:
```
gcc main.cpp
./a.out receive filename_to_create.txt 9999 
./a.out send filename_to_send.txt 127.0.0.1:9999
```

TODO:

* Add comments to better understand the code.
* Test on other platforms. (Only Linux confirmed working.)
* Test that it works across the internet.
* Add a user interface. (GUI?)
* Split the code up into multiple files(?)
* Create a makefile.


Starter code was taken from https://developerweb.net/viewtopic.php?id=5843 and modified for my use cases.