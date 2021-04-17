# Nitro
Nitro allows for users to send and receive large files directly to and from one another.

Use (note that key1 and key2 and optional arguments used for file encryption):
```
make
./Nitro receive filename_to_create port# key1 key2
./Nitro send filename_to_send ip_address_destination:port# key1 key2
```

TODO:

* Add comments to better understand the code.
* Test on other platforms. (Only Linux confirmed working.)
* Test that it works across the internet.
* Add a user interface. (GUI?)
* Split the code up into multiple files(?)
* Create a makefile.
* Add file encryption for security purposes.

Starter code was taken from https://developerweb.net/viewtopic.php?id=5843 and modified for my use cases.