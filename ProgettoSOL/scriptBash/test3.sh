#! /bin/bash

# TERZO FILE DI TEST

# STRESS TEST DEL SERVER


# Avvio server:
./server ./config/config3.txt &

# mantengo il pid del processo per mandare SIGHUP:
pid=$!

sleep 2

sleep 30 && kill -2 ${pid} &

pids=()
# Avvio i client:
for i in {1..10};do
    bash ./scriptBash/test3client.sh &
    pids+=($!)
    sleep 0.1
done

sleep 27

for i in "${pids[@]}";do
    kill -9 ${i}
    wait ${i}
done

wait ${pids}

echo -e -n "Test3: terminazione dei client.\n"

wait ${pid}

echo -e -n "Fine Test3.\n"

exit 0