# -*- coding: utf-8 -*-
import http.client    #修改引用的模块
import urllib

reqheaders={'MobileType':'Android'}  

reqconn=http.client.HTTPConnection("127.0.0.1")  #修改对应的方法
reqconn.request("GET", "/buffer", '{"type":"get_resource_info","url":"https://img2.baido.com/it/u=1506121011,1888356275&fm=26&fmt=auto&gp=0.jpg"}', reqheaders)
res=reqconn.getresponse()
print (res.status,  res.reason)
print (res.msg)
print (res.read())