echo "Running unit tests:"

for i in $1/*.bin
do
    if test -f $i
    then
        if ./$i
        then
            echo $i PASS
        else
            echo "ERROR in test $i:"
            exit 1
        fi
    fi
done

echo "${txtbld}$(tput setaf 2)All unit tests passed.$(tput sgr0)"
