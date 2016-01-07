FROM     ubuntu:14.04
ENV DEBIAN_FRONTEND noninteractive
RUN     apt-get -y install software-properties-common
RUN     apt-get -y install python2.7
RUN     apt-get update -y
RUN     apt-get -y install supervisor
RUN     apt-get -y install python-pip
RUN     pip install paho-mqtt
RUN     apt-get -y install --no-install-recommends openssh-server

# To handle non symlink stupidity
# run 'docker build -t pastcompute/mqtt2graphite -f Dockerfile ../..'

ADD     Docker/mqtt2graphite/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

ADD     ext/mqtt2graphite/mqtt2graphite.py /usr/local/bin/mqtt2graphite.py
ADD     data/mqtt2graphite-sf.map /home/sentrifarm/mqtt2graphite-sf.map

RUN     chmod 0755 /usr/local/bin/mqtt2graphite.py

# We would like to run the following:
#   MQTT_HOST=1.2.3.4 python2.7 mqtt2graphite.py mqtt2graphite-sf.map
# This will connect to MQTT_HOST as a subscriber and push into Carbon at 127.0.0.1:2003
#
# Note: setting up sshd inside the docker is a freaking PITA
# Note: for the moment live with port forwarding setups back insides

CMD     ["/usr/bin/supervisord"]
