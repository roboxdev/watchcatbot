import os
import logging

from datetime import datetime, timedelta

from telegram import (
    Bot,
    InlineQueryResultArticle,
    InputVenueMessageContent,
    InlineKeyboardButton,
    InlineKeyboardMarkup,
    ReplyKeyboardMarkup,
    ChatAction,
)
from telegram.ext import (
    Dispatcher,
    Updater,
    InlineQueryHandler,
    CallbackQueryHandler,
    CommandHandler,
    Filters,
)
from telegram.update import Update

import requests

logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.DEBUG)

BOT_TOKEN = os.environ['BOT_TOKEN']
BLYNK_REQUEST_URL = os.environ['BLYNK_REQUEST_URL']
WEBHOOK_URL = os.environ.get('WEBHOOK_URL')

ADDRESS = os.environ.get('ADDRESS', '')
ADDRESS_DETAILS = os.environ.get('ADDRESS_DETAILS', '')
LATITUDE = float(os.environ.get('LATITUDE', '0'))
LONGITUDE = float(os.environ.get('LONGITUDE', '0'))

LOG_CHAT_ID = os.environ.get('LOG_CHAT_ID')
SUPERUSER_ID = os.environ.get('SUPERUSER_ID')
USERS_WHITELIST = os.environ['USERS_WHITELIST']  # comma-separated ids: "123, 456"

whitelist = [int(tg_id) for tg_id in USERS_WHITELIST.split(',')]


class WatchcatBot:
    def log(self, message, user=None, silent=False):
        bot = Bot(BOT_TOKEN)
        text = message
        if user:
            contact_info = f'<b><a href="tg://user?id={user.id}">{user.full_name}</a></b> ' \
                           f'<i>(id <a href="https://t.me/@id{user.id}">{user.id}</a>)</i>'
            if username := user.username:
                contact_info += f' @{username}'
            text = f'{contact_info}\n' \
                   f'\n' \
                   f'{text}'
        bot.send_message(
            chat_id=LOG_CHAT_ID,
            text=text,
            parse_mode='HTML',
            disable_notification=silent,
        )

    @staticmethod
    def open_entrance_door(duration=20):
        requests.put(BLYNK_REQUEST_URL, json=[duration])

    def inline_query_callback(self, update, context):
        if update.effective_user.id not in whitelist:
            update.inline_query.answer(results=[])
            return

        message = InputVenueMessageContent(
            title=ADDRESS,
            address=ADDRESS_DETAILS,
            latitude=LATITUDE,
            longitude=LONGITUDE,
        )
        open_button = InlineKeyboardButton(
            text='Открыть дверь подъезда',
            callback_data=f'open|{int((datetime.now() + timedelta(days=1)).timestamp())}',
        )
        result_open_button = InlineQueryResultArticle(
            id='1',
            title='Кнопка на сутки',
            description=f'{ADDRESS}, открыть дверь',
            input_message_content=message,
            reply_markup=InlineKeyboardMarkup([[
                open_button
            ]])
        )
        open_button_week = InlineKeyboardButton(
            text='Открыть дверь подъезда',
            callback_data=f'open|{int((datetime.now() + timedelta(weeks=1)).timestamp())}',
        )
        result_open_button_week = InlineQueryResultArticle(
            id='2',
            title='Кнопка на неделю',
            description=f'{ADDRESS}, открыть дверь',
            input_message_content=message,
            reply_markup=InlineKeyboardMarkup([[
                open_button_week
            ]])
        )
        open_button_quarter = InlineKeyboardButton(
            text='Открыть дверь подъезда',
            callback_data=f'open|{int((datetime.now() + timedelta(weeks=13)).timestamp())}',
        )
        result_open_button_quarter = InlineQueryResultArticle(
            id='3',
            title='Кнопка на квартал',
            description=f'{ADDRESS}, открыть дверь',
            input_message_content=message,
            reply_markup=InlineKeyboardMarkup([[
                open_button_quarter
            ]])
        )
        update.inline_query.answer(
            results=[
                result_open_button,
                result_open_button_week,
                result_open_button_quarter,
            ],
            is_personal=True,
            cache_time=0,
        )

    def open_the_door_query_callback(self, update, context):
        command, ts = update.callback_query.data.split('|')
        button_valid = datetime.utcnow() < datetime.utcfromtimestamp(int(ts))
        if command == 'open':
            user = update.effective_user
            if button_valid:
                self.open_entrance_door()
                update.callback_query.answer('Заходи')
                self.log(message='Открыл дверь', user=user)
            else:
                update.callback_query.edit_message_reply_markup()
                update.callback_query.answer('Кнопка протухла')
                self.log(message='Кнопка протухла', user=user)

    def open_entrance_door_callback(self, update, context):
        message = update.message
        chat_id = message.chat.id
        bot = context.bot
        bot.send_chat_action(chat_id=chat_id, action=ChatAction.TYPING)
        self.open_entrance_door()
        reply_markup = ReplyKeyboardMarkup([['/open_entrance_door']])
        message.reply_text(text='Заходи', reply_markup=reply_markup)

    def set_handlers(self, dispatcher):
        inline_handler = InlineQueryHandler(
            callback=self.inline_query_callback,
        )
        dispatcher.add_handler(inline_handler)
        open_the_door_query_handler = CallbackQueryHandler(
            callback=self.open_the_door_query_callback,
        )
        dispatcher.add_handler(open_the_door_query_handler)
        open_entrance_door_handler = CommandHandler(
            command='open_entrance_door',
            callback=self.open_entrance_door_callback,
            filters=Filters.user(user_id=whitelist)
        )
        dispatcher.add_handler(open_entrance_door_handler)

    def setup_dispatcher(self, token):
        bot = Bot(token)
        dispatcher = Dispatcher(bot, None, workers=0)
        self.set_handlers(dispatcher)
        return dispatcher

    @classmethod
    def process_bot_request(cls, data):
        bot = cls()
        dispatcher = bot.setup_dispatcher(token=BOT_TOKEN)
        update = Update.de_json(data, dispatcher.bot)
        dispatcher.process_update(update)
        return 'ok'

    @classmethod
    def process_request(cls, data):
        # TODO
        return 'OK'

    @classmethod
    def start_polling(cls):
        updater = Updater(token=BOT_TOKEN)
        bot = cls()
        bot.set_handlers(updater.dispatcher)
        updater.bot.delete_webhook()
        updater.start_polling()

    @classmethod
    def set_webhook(cls):
        updater = Updater(token=BOT_TOKEN)
        updater.bot.set_webhook(WEBHOOK_URL)


def main(request):
    data = request.args if request.method == 'GET' else request.get_json(force=True)
    if 'update_id' in data:
        return WatchcatBot.process_bot_request(data)
    else:
        return WatchcatBot.process_request(data)


if __name__ == '__main__':
    WatchcatBot.set_webhook()
    # DEBUG ONLY
    WatchcatBot.start_polling()
