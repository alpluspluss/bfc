#!/bin/bash

# create a temp directory for signing
mkdir -p ./temp_sign

# copy the executable to the temp directory
cp ./bfc ./temp_sign/

# sign the executable in the temp directory
codesign --force --options runtime --sign - --entitlements ./resources/entitlements.plist ./temp_sign/bfc --verbose

# check if signing was successful
if [ $? -eq 0 ]; then
    echo "Signing successful."
    # move the signed executable back
    mv ./temp_sign/bfc ./bfc
    rm -rf ./temp_sign
else
    echo "Signing failed."
    rm -rf ./temp_sign
    exit 1
fi

echo "Done."