#!/bin/sh
if ! test -d $HOME/.divine; then 
    mkdir $HOME/.divine
    mkdir $HOME/.divine/user
    cat > $HOME/.divine/algorithms <<EOF;
owcty: executable: owcty
owcty: description: Explicit OWCTY
owcty: needs-property: yes
owcty: short: Explicit OWCTY
owcty-reversed: executable: owcty_reversed
owcty-reversed: description: OWCTY Reversed
owcty-reversed: needs-property: yes
owcty-reversed: short: OWCTY Reversed
negative-cycle-detection: executable: negative_cycle_detection
negative-cycle-detection: description: Negative Cycle Detection
negative-cycle-detection: needs-property: yes
negative-cycle-detection: short: Negative Cycle
property-driven-ndfs: executable: property_driven_ndfs
property-driven-ndfs: description: Property Driven Nested DFS
property-driven-ndfs: needs-property: yes
property-driven-ndfs: short: Property Driven NDFS
bledge: executable: bledge
bledge: description: Back Level Edge Based Cycle Detection
bledge: needs-property: yes
bledge: short: BL-Edge
distr-reach: executable: distr_reachability
distr-reach: description: Distributed Reachability
distr-reach: needs-property: no
distr-reach: short: Reachability
token-based-ndfs: executable: token_based_ndfs
token-based-ndfs: description: Token Based Nested DFS
token-based-ndfs: needs-property: yes
token-based-ndfs: short: Token Based NDFS
distr-map: executable: distr_map
distr-map: description: Maximal Accepting Predecessor
distr-map: needs-property: yes
distr-map: short: MAP
EOF

    cat > $HOME/.divine/clusters <<EOF;
DiVinE: host: anna.fi.muni.cz
DiVinE: type: pbs
DiVinE: login: divineuser
DiVinE: work-directory: /home/divineuser/divine/dwi/store/work
DiVinE: computers: psyche01: load: 0.5
DiVinE: computers: psyche01: cpu-count: 2
DiVinE: computers: psyche02: load: 0.5
DiVinE: computers: psyche02: cpu-count: 2
DiVinE: computers: psyche03: load: 0.5
DiVinE: computers: psyche03: cpu-count: 2
DiVinE: computers: psyche04: load: 0.5
DiVinE: computers: psyche04: cpu-count: 2
DiVinE: computers: psyche05: load: 0.5
DiVinE: computers: psyche05: cpu-count: 2
DiVinE: computers: psyche06: load: 0.5
DiVinE: computers: psyche06: cpu-count: 2
DiVinE: computers: psyche07: load: 0.5
DiVinE: computers: psyche07: cpu-count: 2
DiVinE: computers: psyche08: load: 0.5
DiVinE: computers: psyche08: cpu-count: 2
DiVinE: computers: psyche09: load: 0.5
DiVinE: computers: psyche09: cpu-count: 2
DiVinE: computers: psyche10: load: 0.5
DiVinE: computers: psyche10: cpu-count: 2
DiVinE: computers: psyche11: load: 0.5
DiVinE: computers: psyche11: cpu-count: 2
DiVinE: computers: psyche12: load: 0.5
DiVinE: computers: psyche12: cpu-count: 2
DiVinE: computers: psyche13: load: 0.5
DiVinE: computers: psyche13: cpu-count: 2
DiVinE: computers: psyche14: load: 0.5
DiVinE: computers: psyche14: cpu-count: 2
DiVinE: computers: psyche15: load: 0.5
DiVinE: computers: psyche15: cpu-count: 2
DiVinE: computers: psyche16: load: 0.5
DiVinE: computers: psyche16: cpu-count: 2
DiVinE: computers: psyche17: load: 0.5
DiVinE: computers: psyche17: cpu-count: 2
DiVinE: computers: psyche18: load: 0.5
DiVinE: computers: psyche18: cpu-count: 2
DiVinE: computers: psyche19: load: 0.5
DiVinE: computers: psyche19: cpu-count: 2
DiVinE: computers: psyche20: load: 0.5
DiVinE: computers: psyche20: cpu-count: 2
DiVinE: computers: psyche21: load: 0.5
DiVinE: computers: psyche21: cpu-count: 2
DiVinE: computers: psyche22: load: 0.5
DiVinE: computers: psyche22: cpu-count: 2
EOF
fi

java -Djavax.net.ssl.trustStore="truststore" -Djavax.net.ssl.trustStorePassword=divine -cp "@DWI_JAR_PATH@dwi.jar" divine.client.Client --imagedir "@DWI_IMAGE_PATH@" --datadir "$HOME/.divine" $@
