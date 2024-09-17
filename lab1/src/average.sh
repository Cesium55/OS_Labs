sm=0

ct=0

for i in $@

do

sm=$(echo "$sm + $i" | bc)

ct=$((ct+1))

done

echo "scale=2;$sm / $ct" | bc
