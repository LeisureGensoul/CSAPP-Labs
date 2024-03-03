phase 1: 
> ./hex2raw -i Solution/ctarget_1.txt | ./ctarget -q  

phase 2:
> objdump -d ./ctarget>>ctarget.s
> gcc -c Solution/ctarget_level2.s  
> objdump -d ./ctarget_level2.o  
> ...  
> ./hex2raw -i Solution/ctarget_2.txt | ./ctarget -q 

phase 3:
> ./hex2raw -i Solution/ctarget_3.txt | ./ctarget -q

phase 4:
> ./hex2raw -i Solution/rtarget_2.txt | ./rtarget -q

phase 5:
> ./hex2raw -i Solution/rtarget_3.txt | ./rtarget -q