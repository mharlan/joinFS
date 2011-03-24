rm valgrind.log
valgrind --tool=memcheck --leak-check=yes ./joinfs queries/ data/ mount/ >> valgrind.log
