#!/bin/bash

DIRECTORY=$(dirname $0)
COMMAND=$(basename $0 .sh)

case $# in
  ( 1 )
    DATA=$(ls $DIRECTORY/$COMMAND.$1.*.json)
    PREP=$DIRECTORY/$(basename $DATA .json).prep
    ;;
  ( * )
    echo
    echo Usage ":" bash $COMMAND.sh TEST
    echo
    echo TEST is one of the following options ...
    for i in $(ls $DIRECTORY/$COMMAND.*.json)
    do
      echo "    " $(basename $i .json | cut --delimiter=\. --output-delimiter=' ' --fields=2-3 - | tr '-' ' ')
    done
    echo
    exit
    ;;
esac

case $1 in
  ( '01' )
    TOPIC=enrollment.created
    HMAC=rubbish
    SUBDOMAIN=filth
    ;;
  ( * )
    TOPIC=enrollment.created
    HMAC=$(cat $DATA | tr -d '\n' | openssl sha256 -mac HMAC -macopt key:$(drush vget thinkific_api | head -1 | cut -d' ' -f2) | cut -d' ' -f2)
    SUBDOMAIN=$(drush vget thinkific_subdomain | head -1 | cut -d' ' -f2)
    ;;
esac

if [[ -f $PREP ]]
then
  echo
  cat $PREP
  echo
  echo
  echo Press ENTER to run the test ...
  read
fi

echo

curl \
  --insecure \
  --request POST \
  --header "Content-Type: application/json" \
  --header "X-Thinkific-Topic: $TOPIC" \
  --header "X-Thinkific-Hmac-Sha256: $HMAC" \
  --header "X-Thinkific-Subdomain: $SUBDOMAIN" \
  --data @$DATA \
  --fail \
    https://citybibleforum.test/webhook/thinkific

echo Exit code is $?

echo
echo
cat $DIRECTORY/$COMMAND.$1.*.txt
echo
echo
