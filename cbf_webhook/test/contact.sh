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
    HEADERNAME=Webhook-Agent
    HEADERVALUE=unknown-agent
    ;;
  ( * )
    HEADERNAME=WebhookAgent
    HEADERVALUE=UkMQ6bTDDNWUTlFJ2oP23vMm
    ;;
esac

if [[ -f $PREP ]]
then
  echo
  cat $PREP
  echo
  echo Run test ...
  read
fi

echo

cd $DIRECTORY

case $(pwd) in
  ( /var/www/civicrm/* )
    SITE=citybibleforum.civicrm
    ;;
  ( * )
    SITE=citybibleforum.test
    ;;
esac

curl \
  --insecure \
  --request POST \
  --header "Content-Type: application/json" \
  --header "$HEADERNAME: $HEADERVALUE" \
  --data @$DATA \
  --fail \
    https://$SITE/webhook/$COMMAND

echo Exit code is $?

cat $DIRECTORY/$COMMAND.$1.*.txt
