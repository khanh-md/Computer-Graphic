Make sure the "include" folder contain the header files for GLFW and GLEW, 
and the "lib" folder contain the .lib files for those libraries.

The dll files for GLFW and GLEW should be in the same directory as the program.

To run the code, change the paths in the makefile to point to the "include" and "lib" folder
inside this directory. Then, in terminal, type:
- make 
- ./rt2.exe

This will run the program and open a window that show the result.

If makefile does not work, run this command manually (change the paths correspondingly):
- g++ -g -std=c++17 -o rt2.exe -IC:\path\to\cap5705_a2\include -LC:\path\to\cap5705_a2\lib rt2.cpp -lglew32 -lglfw3dll -lopengl32



To change camera perspective, simply press "p".

To create a file of render images, make a folder called "frames". In the code, uncomment lines 387 and 463-466.
To create a movie from the render images, make sure FFmpeg is installed and run this inside "frames" folder:
- ffmpeg -framerate 30 -i frame_%04d.png -c:v libx264 -pix_fmt yuv420p output.mp4