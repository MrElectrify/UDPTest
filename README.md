# UDPTest
UDPTest is a network testing utility that measures stability at certain performance levels. It is similar to iperf, but provides more statistics in regards to variance and latency.

```
Usage:
  UDPTest launcher [OPTION...]

  -s, --server          Server mode
  -c, --client          Client mode
  -a, --address arg     Server address (default: 127.0.0.1)
  -p, --port arg        Server port (default: 5601)
  -b, --bitrate arg     The bitrate per second to send (default: 1M)
  -r, --packetrate arg  The rate of packets per second to send (default: 100)
  -t, --time arg        The total time to test in seconds (default: 10)
  -h, --help            Display this help message
  ```
