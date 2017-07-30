# -*- coding: UTF-8 -*-
__author__ = 'songchunjie'

from scrapy import Request
import sys
import urllib
from lxml import etree
from ..items import jobsItem

from scrapy.spiders import Spider

class DmozSpider(Spider):
    name = "dmoz"
    allowed_domains = ["dmoz.org"]
    num = 2
    headers = {'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
               'Accept-Encoding': 'gzip,deflate,sdch',
               'Accept-Language': 'zh-CN,zh;q=0.8',
               'Cache-Control': 'max-age=0',
               'Connection': 'keep-alive',
               'Content-Type': 'application/x-www-form-urlencoded'
               }

    def start_requests(self):
        url1 = 'http://search.51job.com/list/010000,000000,0000,00,9,99,C%2520%25E5%25BC%2580%25E5%258F%2591,2,'
        url2 = '.html?lang=c&stype=&postchannel=0000&workyear=99&cotype=99&degreefrom=99&jobterm=99&companysize=99&providesalary=99&lonlat=0%2C0&radius=-1&ord_field=0&confirmdate=9&fromType=&dibiaoid=0&address=&line=&specialarea=00&from=&welfare='
        url =url1 +str(self.num)+url2
        yield Request(url, headers=self.headers)

    '''
        start_urls = [
        "http://search.51job.com/list/010000,000000,0000,00,9,99,C%2520%25E5%25BC%2580%25E5%258F%2591,2,1.html?lang=c&stype=&postchannel=0000&workyear=99&cotype=99&degreefrom=99&jobterm=99&companysize=99&providesalary=99&lonlat=0%2C0&radius=-1&ord_field=0&confirmdate=9&fromType=&dibiaoid=0&address=&line=&specialarea=00&from=&welfare=",
    ]
    '''
    '''
    def parse(self, response):
        filename = response.url.split("/")[-2]
        with open(filename, 'wb') as f:
            f.write(response.body)
    '''
    '''
    class jobsItem(scrapy.Item):
    jobname = scrapy.Field()
    jobcommpany = scrapy.Field()
    jobaddr = scrapy.Field()
    jobpay = scrapy.Field()
    jobtime = scrapy.Field()
    '''
    def  parse(self, response):
        item =jobsItem()
        #all_url=response.xpath('//*[@id="resultList"]/*')
        #//*[@id="resultList"]/div[3]/p/span/a
        for i in range(3,10):
            #//*[@id="resultList"]/div[3]/p/span/a
            item['jobname'] = response.xpath('.//div['+str(i)+']/p/span/a/@title').extract()
            item['jobcommpany'] = response.xpath('.//div['+str(i)+']/p/span[1]/a/@title').extract()
            #print item['jobname'][0].encode('gbk')
            #print item['jobcommpany'][0].encode('gbk')
            yield item
        self.num+=1
        url3 = 'http://search.51job.com/list/010000,000000,0000,00,9,99,C%2520%25E5%25BC%2580%25E5%258F%2591,2,'
        url4 = '.html?lang=c&stype=&postchannel=0000&workyear=99&cotype=99&degreefrom=99&jobterm=99&companysize=99&providesalary=99&lonlat=0%2C0&radius=-1&ord_field=0&confirmdate=9&fromType=&dibiaoid=0&address=&line=&specialarea=00&from=&welfare='
        url =url3 +str(self.num)+url4
            #print 'SSSSSSSSSSSSSSSSSS:',url
        yield Request(url, headers=self.headers,callback=self.parse)