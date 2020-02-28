xhost +local:

#docker run -v `pwd`/py_test_scripts:/app/py_test_scripts -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package:1.0 /bin/bash
#docker run -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v ${RES_DIR}:/app/results -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package:1.0 /bin/bash
docker run -v `pwd`/ci:/ci -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro test /bin/bash
