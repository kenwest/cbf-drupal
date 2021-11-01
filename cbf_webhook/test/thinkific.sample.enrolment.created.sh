#!/bin/bash

curl \
  --insecure \
  --request POST \
  --header "Content-Type: application/json" \
  --header "X-Thinkific-Topic: enrollment.created" \
  --header "X-Thinkific-Hmac-Sha256: some-hex-encoded-weird-token" \
  --header "X-Thinkific-Subdomain: passion-to-profit" \
  --data @/home/ken/Downloads/thinkific.sample.enrolment.created.json \
  --fail \
    https://citybibleforum.test/webhook/thinkific
echo Exit code is $?
