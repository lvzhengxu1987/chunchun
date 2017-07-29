# -*- coding: UTF-8 -*-
__author__ = 'songchunjie'

import scrapy
import sys
import urllib
from lxml import etree

class DmozSpider(scrapy.Spider):
    name = "dmoz"
    allowed_domains = ["dmoz.org"]
    start_urls = [
        "http://search.51job.com/list/010000,000000,0000,00,9,99,C%2520%25E5%25BC%2580%25E5%258F%2591,2,1.html?lang=c&stype=&postchannel=0000&workyear=99&cotype=99&degreefrom=99&jobterm=99&companysize=99&providesalary=99&lonlat=0%2C0&radius=-1&ord_field=0&confirmdate=9&fromType=&dibiaoid=0&address=&line=&specialarea=00&from=&welfare=",
    ]
    '''
    def parse(self, response):
        filename = response.url.split("/")[-2]
        with open(filename, 'wb') as f:
            f.write(response.body)
    '''
    def  parse(self, response):
        a=response.xpath('//*[@id="resultList"]/div[3]/p/span/a/@title').extract()
        b=response.xpath('//*[@id="resultList"]/div[3]/p/span/a/@href').extract()
        print a[0].encode('gbk')
        print b[0].encode('gbk')
        p = urllib.urlopen(b[0])
        html1 = p.read()
        s = etree.HTML(html1)
        #print html1
        b1=response.xpath('../html/body/div[2]/div[2]/div[3]/div[4]/div/text()[2]').extract()

        print b1