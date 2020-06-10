stats=""
stats=$1
size=$2
files=$(ls result*)
result_file_name=result_all_$stats.txt
rm $result_file_name
for file in $files; do
    echo $file >>$result_file_name
    for stat in $stats; do

        echo $stat >>$result_file_name
        awk '/'$stat'/{print $2}' $file | tail -n $size >>$result_file_name
    done
done
