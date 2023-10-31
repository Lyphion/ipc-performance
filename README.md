# Performance analysis of inter-process Communication

This project contains data structures and methods for inter-process communication.
It aims to analyse the impact on the performance of different communication techniques.

## Structure

All communication will be managed via a common communication interface, method and object structure.
When sending messages a header will be created and prepended before the actual message to make it identifiable for the receiver. This header consists of an `id`, `type`, `size` and `timestamp`. Afterward the actual message will be appended. The structure of a message depends on the implementation.
