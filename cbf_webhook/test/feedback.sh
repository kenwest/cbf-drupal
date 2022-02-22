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
    HEADERNAME=Dummy
    HEADERVALUE=dummy
    ;;
  ( '02' )
    HEADERNAME=Webhook-Agent
    HEADERVALUE=unknown-agent
    ;;
  ( * )
    HEADERNAME=WebhookAgent
    HEADERVALUE=SC5izvuvijKFYo8d7smlP0X6
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

curl \
  --insecure \
  --request POST \
  --header "Content-Type: application/json" \
  --header "$HEADERNAME: $HEADERVALUE" \
  --data @$DATA \
  --fail \
    https://citybibleforum.test/webhook/$COMMAND

echo Exit code is $?

cat $DIRECTORY/$COMMAND.$1.*.txt
