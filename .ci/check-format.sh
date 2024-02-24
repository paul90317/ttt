#!/usr/bin/env bash

CLANG_FORMATER=clang-format
CLANG_FORMATER_FLAGS=-style=file

# check clang-format installed
command -v "$CLANG_FORMATER" > /dev/null
ret=$?

if [[ $ret -ne 0 ]]; then
    echo -e "\e[31;1m$CLANG_FORMATER not installed...\e[0m"
    exit $ret
else
    # make sure version >= 12
    $CLANG_FORMATER --version | grep -E ".*version (1[2-9]|[2-9][0-9]+).*" >> /dev/null
    ret=$?

    if [[ $ret -ne 0 ]]; then
        echo -e "\e[31;1m$CLANG_FORMATER version should >= 12 ...\e[0m"
        exit $ret
    fi
fi

# check format
SOURCES=$(find $(git rev-parse --show-toplevel) -name '*.c' -o -name '*.h')

for file in ${SOURCES};
do
    $CLANG_FORMATER $CLANG_FORMATER_FLAGS ${file} > expected-format
    diff -u -p --label="${file}" --label="expected coding style" ${file} expected-format
done
rm expected-format
ret2=$($CLANG_FORMATER --output-replacements-xml ${SOURCES} | grep -E -c "</replacement>")

# check if files end with new line
ret3=$(find $SOURCES -type f | xargs -L1 bash -c 'test "$(tail -c1 "$0")" && echo "No newline at end of $0"')
echo "$ret3"
exit $(($ret || $ret2 || !`test "$ret3"; echo $?`))
