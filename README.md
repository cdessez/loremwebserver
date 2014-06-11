Those scripts and programs provide tools to create a dummy fake http flow and measure the throughput between two nodes:

- loremwebserver.py: a very minimal python web server that serves any request with a fake non-ending file with a 'Lorem Ipsum' content. This is actually very slow and was recoded in C (cf next point).

- cloremwebserver/: the same as above but coded in C. It is actually much much faster than the previous one.

- mywget.sh: a dummy client http client that drops all it receives in /dev/null. It actually just sets the right parameter for the 'wget' command. Useful to display the throughput. When used with cloremwebserver, equivalent computing capabilities on both sides and a very high speed link (or a loopback network interface), it is the bottleneck.

- mycurl.sh: the same as above, but using 'curl' instead of 'wget'. It is actually a bit faster than the previous version.

- dummywget/: the same as above, but coded in C. Actually much much faster than the previous ones.

Those tools were coded for linux.
