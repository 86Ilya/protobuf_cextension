FROM centos

# Set the working directory to /root/cextension
WORKDIR /root/cextension

# Copy the current directory contents into the container at /root/app
COPY . /root/cextension

RUN yum -y install epel-release && yum clean all
RUN yum -y install make protobuf protobuf-c protobuf-c-compiler gcc mc python-pip \
		   protobuf-c-devel python-devel python-setuptools gdb 
RUN pip install -r requirements.txt
# ENTRYPOINT ["sh", "/root/cextension/start.sh"]
