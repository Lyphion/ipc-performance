# Performance analysis of inter-process Communication

This project contains data structures and methods for inter-process communication.
It aims to analyse the impact on the performance of different communication techniques on Unix systems.

## Communication structure

All communication will be managed via a common communication interface ([ICommunicationHandler](include%2Fhandler%2Fcommunication_handler.hpp)), method and object structure ([IDataObject](include%2Fobject%2Fdata_object.hpp)).
When sending messages a header ([DataHeader](include%2Fobject%2Fdata_header.hpp)) will be created and prepended before the actual message, to make it identifiable for the receiver. This header consists of an `id`, `type`, `size` and `timestamp`. Afterward the actual message will be appended. The structure of the serialized message depends on the implementation.

Currently, the following handlers are implemented:
- [Datagram Socket](include%2Fhandler%2Fdatagram_socket.hpp) (Unix and Internet domain)
- [Stream Socket](include%2Fhandler%2Fstream_socket.hpp) (Unix and Internet domain)
- [DBus](include%2Fhandler%2Fdbus.hpp)
- [Fifo/Named pipe](include%2Fhandler%2Ffifo.hpp)
- [Posix Message Queue](include%2Fhandler%2Fmessage_queue.hpp)
- [Shared file](include%2Fhandler%2Fshared_file.hpp)
- [Shared memory](include%2Fhandler%2Fshared_memory.hpp) (Posix shared memory and Memory mapped file)

All data object must be defined via a [DataType](include%2Fobject%2Fdata_type.hpp), as an implementation of ([IDataObject](include%2Fobject%2Fdata_object.hpp)) and as a possible return type via [ICommunicationHandler::DataObject](include%2Fhandler%2Fcommunication_handler.hpp). The utility file [utility.hpp](include%2Futility.hpp) will help to deserialize each object by its type.
