# Elysabettian - C++ implementation of the Elysabettian Scrpting Language

## 1. Introduction
**Elysabettian** is a simple, fast C++17 implementation of the Elysabettian programming language.<br>
Elysabettian is a small footprint scripting language which aims at ease of use without much penalty on overall performance.<br><br>
Elysabettian uses a JavaScript-like syntax, with dyanmic types. As of today, objects, strings, numbers and nullable types are supported. Future releases of the language will aim to improve functionalities without implementing any breaking change.
## 2. Installation
To **download and install** ***Elysabettian*** you need to have ```git``` and ```cmake``` installed on your operating system, as well as a fully compliant C++17 compiler. This Virtual Machine uses ```std::variant``` to store internal data types, and so the compiler needs to be **fully compliant** with the C++17 standard.  
In order to compile this code, you will need ```vcpkg``` installed on your machine, and setup correctly. This is needed to install ```fmt```, which is the only dependency that this project needs. To install ```fmt```, run the setup script for your operating system, available in the ```scripts``` directory.  

To build the code, create a ```build``` directory for CMake, ```cd``` into it and run ```cmake .. --preset=<Preset Name>```. Available presets are ```Debug``` and ```Release```.
## 3. Goals
The goals of this programming language are simple: improve C++ proficiency and make the understanding of programming languages simpler to Computer Science novices. This will be used as a Thesis project for my Bachelor of Science in Computer Science.