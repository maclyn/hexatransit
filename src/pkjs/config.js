module.exports = [
    {
        "type": "heading",
        "defaultValue": "Settings"
    },
    {
        "type": "text",
        "defaultValue": "Customize"
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Preferences"
            },
            {
                "type": "toggle",
                "messageKey": "PowerMode",
                "label": "High (update every second) or low (update every 15 seconds) power use mode",
                "defaultValue": true
            },
            {
                "type": "toggle",
                "messageKey": "GhostHours",
                "label": "Ghost trails for hours (uses more power)",
                "defaultValue": true
            },
            {
                "type": "toggle",
                "messageKey": "GhostSeconds",
                "label": "Ghost trails for seconds (uses more power)",
                "defaultValue": true
            },
            {
                "type": "toggle",
                "messageKey": "HourlyVibrate",
                "label": "Vibrate every hour",
                "defaultValue": true
            },
            {
                "type": "toggle",
                "messageKey": "DisconnectVibrate",
                "label": "Vibrate on disconnected from phone",
                "defaultValue": true
            },
        ]
    },
    {
        "type": "submit",
        "defaultValue": "Save"
    }
];