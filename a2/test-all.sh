#! /bin/bash

xx=$(tput setaf 1)$(tput bold)"XX"$(tput sgr0)
ok=$(tput setaf 2)$(tput bold)"OK"$(tput sgr0)
all_ok=true;


for output in ./samples/*.out; do
  app="$(basename $output .out)";

  if ! make $app; then
    echo -e $xx" failed to build $app";
  else

    input="./samples/$(basename $input .out).in"
    if [ ! -f $input ]; then
      exe="./$app";
    else
      exe="./$app < $($input)";
    fi

    dcount=$("$exe" | diff -U 0 $output - | grep ^@ | wc -l)
    if [ "$dcount" -eq "0" ]; then
      echo -e $ok": sample '"$(basename $app)"'";
    else
      echo -e $xx": sample '"$(basename $app)"'";
      echo -e "\t$dcount mismatched lines";
      all_ok=false;
    fi
  fi

done

if [ all_ok ]; then
  echo -e $(tput setaf 2)$(tput bold)"ALL OK"$(tput sgr0);
fi
