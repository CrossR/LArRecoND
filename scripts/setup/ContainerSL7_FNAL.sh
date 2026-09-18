BIND_PATHS="/cvmfs,/exp,/nashome,/opt,/run/user,/etc/hostname,/etc/hosts,/etc/krb5.conf"

if [ -d "/pnfs/dune" ]; then
    BIND_PATHS="${BIND_PATHS},/pnfs/dune"
fi

apptainer shell --env PS1="C@\h:\W$ " --shell=/bin/bash -B ${BIND_PATHS} /cvmfs/singularity.opensciencegrid.org/fermilab/fnal-dev-sl7:latest
