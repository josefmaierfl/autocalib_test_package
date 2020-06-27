#!/bin/bash

#Download NGRANSAC
git clone https://github.com/vislearn/ngransac.git
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NGRANSAC_DIR="${SOURCE_DIR}/ngransac"

#Copy into working directory
#cp -R ${NGRANSAC_DIR}/. ${SOURCE_DIR}/
rsync -aP --exclude=.git ${NGRANSAC_DIR}/* ${SOURCE_DIR}/
rm -r -f ${NGRANSAC_DIR}
