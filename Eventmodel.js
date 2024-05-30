const mongoose = require('mongoose')

const Schema = mongoose.Schema

const EventsSchema = new Schema(
    {
        _id : String,
        value: String 
    }
)

const Events = mongoose.model('Events', EventsSchema);
module.exports = Events;
