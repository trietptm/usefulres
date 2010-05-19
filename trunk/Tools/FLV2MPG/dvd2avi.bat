ffmpeg -i VIDEO_TS.VOB -qscale 7 -vcodec libxvid -s 640x360 -r 23.976 -aspect 16:9 -ab 128k -ar 48000 -async 48000 -ac 2 -acodec libmp3lame -f avi -g 300 -bf 2 blues2.avi
pause