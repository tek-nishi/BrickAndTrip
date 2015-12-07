#!/bin/sh

rm ../assets/params.json

rm ../assets/stage01.json
rm ../assets/stage02.json
rm ../assets/stage03.json
rm ../assets/stage04.json
rm ../assets/stage05.json
rm ../assets/stage06.json
rm ../assets/stage07.json
rm ../assets/stage08.json
rm ../assets/stage09.json
rm ../assets/stage10.json
rm ../assets/stage11.json

rm ../assets/startline.json
rm ../assets/finishline.json

rm ../assets/ui_allstageclear.json
rm ../assets/ui_credits.json
rm ../assets/ui_gameover.json
rm ../assets/ui_intro.json
rm ../assets/ui_pause.json
rm ../assets/ui_progress.json
rm ../assets/ui_records.json
rm ../assets/ui_regularstageclear.json
rm ../assets/ui_settings.json
rm ../assets/ui_stageclear.json
rm ../assets/ui_title.json


./filedz params.json ../assets/params.data

./filedz stage01.json ../assets/stage01.data
./filedz stage02.json ../assets/stage02.data
./filedz stage03.json ../assets/stage03.data
./filedz stage04.json ../assets/stage04.data
./filedz stage05.json ../assets/stage05.data
./filedz stage06.json ../assets/stage06.data
./filedz stage07.json ../assets/stage07.data
./filedz stage08.json ../assets/stage08.data
./filedz stage09.json ../assets/stage09.data
./filedz stage10.json ../assets/stage10.data
./filedz stage11.json ../assets/stage11.data

./filedz startline.json ../assets/startline.data
./filedz finishline.json ../assets/finishline.data

#./filedz sandbox.json ../assets/sandbox.data


./filedz ui_allstageclear.json ../assets/ui_allstageclear.data
./filedz ui_credits.json ../assets/ui_credits.data
./filedz ui_gameover.json ../assets/ui_gameover.data
./filedz ui_intro.json ../assets/ui_intro.data
./filedz ui_pause.json ../assets/ui_pause.data
./filedz ui_progress.json ../assets/ui_progress.data
./filedz ui_records.json ../assets/ui_records.data
./filedz ui_regularstageclear.json ../assets/ui_regularstageclear.data
./filedz ui_settings.json ../assets/ui_settings.data
./filedz ui_stageclear.json ../assets/ui_stageclear.data
./filedz ui_title.json ../assets/ui_title.data
