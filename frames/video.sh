ffmpeg -framerate $2 -i %d.bmp -c:v libx264 -crf 18 -pix_fmt yuv420p timelapse.mp4
