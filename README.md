# brew_memory_patcher
Patches the contents of RAM on Qualcomm BREW phones.

This is a quick and nasty little program I whipped up.

Simply put your patches in the textbox and watch it change your luck. Few things in life are guaranteed, but I promise this won't suck!

#Syntax

Each line specifies an address and a byte that gets written there. Example:

.12345678 69

Write the value "$69" to memory address "$12345678".

The period (".") seperates each individual line from each other, allowing for multiple lines. Example:

.001ac924 24
.001ac925 42
.001ac926 D8
.001ac927 01

Or keep it all on a single line if you're on a phone where you can't enter new lines:

.001ac924 24.001ac925 42.001ac926 D8.001ac927 01

On a few phones (namely the LG VX10000) you can write to memory mapped hardware registers using this program. Example:

.8000006C 06
