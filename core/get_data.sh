stats=""
stats=$1
size=$2
rm result_all.txt
files=$(ls result*)
for file in $files; do
    echo $file >>result_all.txt
    for stat in $stats; do
        
        echo $stat >>result_all.txt
        awk '/'$stat'/{print $2}' $file | tail -n $size >>result_all.txt
    done
done
