<?php
define('SMS_REMINDER_BALLOU_URL', 'http://sms2.ballou.se/http/get/SendSms.php');

/**
 * Settings form for Ballou HTTP provider.
 */
function sms_reminder_ballou_http_settings() {
	$form = array();
	
	// Fieldset
	$form['ballou_http'] = array(
		'#type' => 'fieldset',
		'#title' => t('Ballou HTTP'),
		'#collapsible' => TRUE,
	);
	
	// Enabled
	$form['ballou_http']['sms_reminder_ballou_http_enabled'] = array(
		'#type' => 'checkbox',
		'#title' => t('Enable this provider.'),
		'#default_value' => variable_get('sms_reminder_ballou_http_enabled', NULL),
	);
	
	// Host
	$form['ballou_http']['sms_reminder_ballou_http_url'] = array(
		'#type' => 'textfield',
		'#title' => t('URL'),
		'#default_value' => variable_get('sms_reminder_ballou_http_url', SMS_REMINDER_BALLOU_URL),
		'#required' => TRUE,
		'#size' => 60,
		'#maxlength' => 255,
		'#description' => t('The SMS service URL.'),
	);
	
	// Username
	$form['ballou_http']['sms_reminder_ballou_http_username'] = array(
		'#type' => 'textfield',
		'#title' => t('Username'),
		'#default_value' => variable_get('sms_reminder_ballou_http_username', NULL),
		'#required' => TRUE,
		'#size' => 40,
		'#maxlength' => 255,
	);
	
	// Password
	$form['ballou_http']['sms_reminder_ballou_http_password'] = array(
		'#type' => 'textfield',
		'#title' => t('Password'),
		'#default_value' => variable_get('sms_reminder_ballou_http_password', NULL),
		'#required' => TRUE,
		'#size' => 40,
		'#maxlength' => 255,
	);
	
	// Sender name
	$form['ballou_http']['sms_reminder_ballou_http_from'] = array(
		'#type' => 'textfield',
		'#title' => t('Sender name'),
		'#default_value' => variable_get('sms_reminder_ballou_http_from', NULL),
		'#required' => FALSE,
		'#size' => 40,
		'#maxlength' => 255,
		'#description' => t('The name that will appear as sender of the message.'),
	);
	
	return $form;
}

/**
 * Provider specific validation.
 */
function sms_reminder_ballou_http_validate($form, &$form_state) {
}

/**
 * Provider specific submit.
 * @return string Empty string on success or error message on failure.
 */
function sms_reminder_ballou_http_submit($form, &$form_state) {
	// Get reminder settings
	$ids = explode(',', $form_state['values']['sms_reminder']);
	$reminder = _sms_reminder_get_record($ids[0], $ids[1]);
	
	// Calculate send time
	$event_time = $reminder['event_time'];
	
	// Assemble date array
	$date = array(
		'hour' => date('H', $event_time),
		'minute' => date('i', $event_time),
		'second' => 0,
		'month' => date('m', $event_time),
		'day' => date('d', $event_time),
		'year' => date('Y', $event_time),
	);
	
	// Format send time
	switch($form_state['values']['when']) {
		case 'three_days_before' :
			$send_time = date('Y-m-d H:i', mktime($date['hour'], $date['minute'], $date['second'], $date['month'], $date['day']-3, $date['year']));
			break;
		case 'day_before' :
			$send_time = date('Y-m-d H:i', mktime($date['hour'], $date['minute'], $date['second'], $date['month'], $date['day']-1, $date['year']));
			break;
		case 'same_day' :
		$send_time = date('Y-m-d H:i', mktime($date['hour']-2, $date['minute'], $date['second'], $date['month'], $date['day'], $date['year']));
			break;
		case 'now' :
			$send_time = NULL;
			break;
	}

	// Get username and password
	$username = utf8_decode(variable_get('sms_reminder_ballou_http_username', NULL));
	$password = utf8_decode(variable_get('sms_reminder_ballou_http_password', NULL));
	$from = utf8_decode(variable_get('sms_reminder_ballou_http_from', variable_get('site_name', 'Drupal')));
	
	// UTF-8 decode
	$to = utf8_decode($form_state['values']['phone']);
	$message = utf8_decode($reminder['message']);
	
	// Get URL
	$url = variable_get('sms_reminder_ballou_http_url', SMS_REMINDER_BALLOU_HOST);
	
	// Assemble data
	$data = array(
		'UN' => $username, // Username
		'PW' => $password, // Password
		'O' => $from, // Sender
		'D' => $to, // Receiver
		'M' => $message, // Message
	);
	
	// Add send time?
	if(!is_null($send_time)) {
		$data['T'] = $send_time;
	}
	
	// Do HTTP request
	$response = drupal_http_request($url, array(), $method = 'GET', $data);
	
	// Check response code
	switch($response->code) {
		case 200 :
			$ret = '';
			break;
		default :
			$ret = $response->error;
			break;
	}
	
	if(strlen($ret) > 0 ) {
		watchdog('sms_reminder', $ret, NULL, WATCHDOG_ERROR);
	}
	
	return $ret;
}