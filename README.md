Moved to https://git.opendlv.org.

```
$ mkdir grafana-data && sudo chown 472:472 grafana-data
$ docker-compose -d up
$ docker exec -it influxdb influx
create database collectd
```
