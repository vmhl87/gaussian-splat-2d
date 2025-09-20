g++ ../scale.cpp -o scale -O2

for ((i=0; i<$1; i++)); do
	./scale $i.conf $i.bmp 2
done

rm scale
