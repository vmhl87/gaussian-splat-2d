rm -f data.csv

for ((i=0; i<=$1; i++)); do
	echo $i,$(cat $i.stat) >> data.csv
done
