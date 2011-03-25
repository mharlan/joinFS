rm valgrind.log
valgrind -v --tool=memcheck --track-fds=yes --leak-check=yes ./joinfs queries/ data/ mount/ >> valgrind.log
