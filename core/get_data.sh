stats="total_prop total_cycle global_blocked_clause global_blocked_times waiting_watcher_list waiting_watcher_times idle_clause_unit_total idle_clause_unit_times idel_watcher_total idel_watcher_times"

rm result_all.txt
files=$(ls result*)
for file in $files; do
    echo $file >>result_all.txt
    for stat in $stats; do
        
        echo $stat >>result_all.txt
        awk '/'$stat'/{print $2}' $file | tail -n 5 >>result_all.txt
    done
done
