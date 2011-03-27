rm valgrind.log
valgrind -v --tool=memcheck --track-fds=yes --track-origins=yes --leak-check=yes ./joinfs queries/ mount/ >> valgrind.log
