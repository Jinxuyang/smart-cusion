// index.js
// 获取应用实例
const app = getApp()

Page({
  data: {
    //需要修改的地方
    uid:"1065cb19f6d3ced80e2d3a378f861f50",//用户密钥，巴法云控制台获取
    dataTopic:"SmartCusion",//控制led的主题，创客云控制台创建
    controllerTopic:"SmartCusionController",//传输温湿度的主题，创客云控制台创建

    checked: false,     //led的状态记录。默认led关闭

    deviceStatus:"离线",// 显示led是否在线的字符串，默认离线
    temperature:"",//温度值，默认为空
    sitTime: "",
    maxSitTime: "",
    heaterStatus: "off",
    heaterMode: "auto",
    heaterStartTemp: "",
    dataTime:"", 
    
    isShowLoading: false
  },

//屏幕打开时执行的函数
  onLoad() {
    //检查设备是否在线
    this.getDeviceStatus()
    //检查设备是打开还是关闭
    this.getData()
    //设置定时器，每3秒请求一下设备状态
    setInterval(this.getDeviceStatus, 10000)
    setInterval(this.getData, 1000)
  },

  turnHeater() {
    this.loadingShow()
    if(this.data.heaterStatus == "on") {
      this.sendMsg("heater:off")
    } else {
      this.sendMsg("heater:on")
    }
    this.getData()
    setTimeout(this.loadingHide, 3000); 
    
  },
  setMaxSitTime(event) {
    this.sendMsg("maxSitTime:"+event.detail)
  },
  setHeaterStartTemp(event) {
    this.sendMsg("heaterStartTemp:"+event.detail)
  },
  loadingShow() {
    this.setData({
      isShowLoading: true
    })
  },
  loadingHide() {
    this.setData({
      isShowLoading: false
    })
  },
  //请求设备状态,检查设备是否在线
  getDeviceStatus(){
    var that = this
     //api 接口详细说明见巴法云接入文档
    wx.request({
      url: 'https://api.bemfa.com/api/device/v1/status/?', //状态api接口，详见巴法云接入文档
      data: {
        uid: that.data.uid,
        topic: that.data.controllerTopic,
      },
      header: {
        'content-type': "application/x-www-form-urlencoded"
      },
      success (res) {
        console.log(res.data)
        if(res.data.status === "online"){//如果在线
          that.setData({
            deviceStatus:"在线"  //设置状态为在线
          })
        }else{                          //如果不在线
          that.setData({
            deviceStatus:"离线"   //设置状态为离线
          })
        }
        console.log(that.data.device_status)
      }
    })    
  },
  getData(){
    //api 接口详细说明见巴法云接入文档
    var that = this
    wx.request({
      url: 'https://api.bemfa.com/api/device/v1/data/1/get/', //状态api接口，详见巴法云接入文档
      data: {
        uid: that.data.uid,
        topic: that.data.dataTopic,
        num:1
      },
      header: {
        'content-type': "application/x-www-form-urlencoded"
      },
      success (res) {
        console.log(res.data.msg)
        const dataList = res.data.msg.split("#")
        var date = new Date()
        that.setData({
          temperature: dataList[0],
          sitTime: (dataList[1] / 60).toFixed(2),
          maxSitTime: (dataList[2] / 60 / 1000),
          heaterStatus: dataList[3],
          heaterMode: dataList[4],
          heaterStartTemp: dataList[5],
          dataTime: date.toLocaleString() 
        })

      }
    })    
  },
  //发送开关数据
  sendMsg(msg){
    console.log("send msg: " + msg)
    wx.request({
      url: 'https://api.bemfa.com/api/device/v1/data/1/push/get/?', //状态api接口，详见巴法云接入文档
      data: {
        uid: this.data.uid,
        topic: this.data.controllerTopic,
        msg:msg
      },
      header: {
        'content-type': "application/x-www-form-urlencoded"
      },
      success (res) {
      }
    })   
  },
})
