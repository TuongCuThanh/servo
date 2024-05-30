const express = require('express');
const app = express();
const port = 3000;
const mongoose = require('mongoose');
const Events = require('./Eventmodel.js');
const mqtt = require('mqtt');
const shortID = require('shortid');
const path = require('path');

app.use(express.json());

// Kết nối MongoDB
const connectDB = async () => {
  await mongoose.connect(`mongodb+srv://ThanhTuong:tuong12345@cluster0.yk8zhbu.mongodb.net/mydb`, {
    useNewUrlParser: true,
    useUnifiedTopology: true,
  });
  console.log(`The database is connected with ${mongoose.connection.host}`);
};

connectDB();

// Kết nối đến MQTT broker
const brokerUrl = 'mqtts://d461e7a5dbd3402da2b7fccff53666ad.s1.eu.hivemq.cloud';

const client = mqtt.connect(brokerUrl, {
  username: 'ce232',
  password: 'Tuong12345'
});

client.on('connect', () => {
  console.log('Đã kết nối đến MQTT broker');
  const topic = '/smarthome/servo';
  client.subscribe(topic, (err) => {
    if (!err) {
      console.log(`Đã đăng ký chủ đề: ${topic}`);
    } else {
      console.error('Lỗi khi đăng ký chủ đề:', err);
    }
  });
});

client.on('message', async (topic, message) => {
  console.log(`Nhận được tin nhắn từ ${topic}: ${message.toString()}`);

  var message_json = `{"value": "${message}"}`;
  try {
    let data = JSON.parse(message_json);
    if (typeof data !== 'object' || data === null) {
      throw new Error('Dữ liệu không phải là một đối tượng JSON hợp lệ');
    }

    data._id = shortID.generate();
    await saveData(data);
  } catch (error) {
    console.error('Lỗi khi phân tích cú pháp tin nhắn:', error);
  }
});

const saveData = async (data) => {
  data = new Events(data);
  data = await data.save();
  console.log('Saved data:', data);
};

// Phục vụ các tệp tĩnh từ thư mục "public"
app.use(express.static(path.join(__dirname, 'public')));

// Route để phục vụ tệp index.html
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// API endpoint để lưu dữ liệu
app.post('/api/data', async (req, res) => {
  try {
    let data = req.body;
    data._id = shortID.generate();
    await saveData(data);
    res.status(201).json({ message: 'Data saved successfully', data: data });
  } catch (error) {
    res.status(500).json({ message: 'Failed to save data', error: error.message });
  }
});

app.listen(port, () => {
  console.log(`Server is running at http://localhost:${port}`);
});