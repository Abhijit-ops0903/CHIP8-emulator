# CHIP8-emulator
A chip8 emulator built using c and raylib.

Chip8 is one of the most basic type of chips/computer that you can emulate as your first emulation project.

#HOW TO RUN
prerequisite : You need to have opengl and dev libraries for open gl like lgdi32 and opengl32\
1.Open Powershell and go to the project dir\
2.paste this command.\
  gcc chip8.c -o CHIP8.exe -I. -L. -lraylib -lopengl32 -lgdi32 -lwinmm -static-libgcc.\
**.Or you can run the .exe file directly from the terminal by pasting this command.\
  .\CHIP8.exe [ROM file goes here with .ch8 extension].\
  you can download roms from here: https://github.com/dmatlack/chip8/tree/master/roms.
3.Run the file and it should work.\

#DEMO:
1.Tetris
  <img width="1270" height="667" alt="Screenshot 2026-06-24 201843" src="https://github.com/user-attachments/assets/4887dd11-6244-4ae5-ac64-7de704ac692b" />.



  
2.Tank
  <img width="1268" height="658" alt="Screenshot 2026-06-24 201807" src="https://github.com/user-attachments/assets/b2912245-5a3e-4067-ac13-0ce289d6ba7b" />

#RESOURCES USED
1.https://austinmorlan.com/posts/chip8_emulator/ (This doc has the code in c++).\
2.https://en.wikipedia.org/wiki/CHIP-8.\


