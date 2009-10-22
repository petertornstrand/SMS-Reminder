<?php
define('SMS_REMINDER_BALLOU_HOST', 'http://sms2.ballou.se');
define('SMS_REMINDER_BALLOU_PORT', '23000');

/**
 * Settings form for Ballou provider.
 */
function sms_reminder_ballou_settings() {
	$form = array();
	
	// Fieldset
	$form['ballou'] = array(
		'#type' => 'fieldset',
		'#title' => t('Ballou'),
		'#collapsible' => TRUE,
		'#collapsed' => TRUE,
	);
	
	// Enabled
	$form['ballou']['sms_reminder_ballou_enabled'] = array(
		'#type' => 'checkbox',
		'#title' => t('Enable this provider.'),
		'#default_value' => variable_get('sms_reminder_ballou_enabled', NULL),
	);
	
	// Host
	$form['ballou']['sms_reminder_ballou_host'] = array(
		'#type' => 'textfield',
		'#title' => t('Host'),
		'#default_value' => variable_get('sms_reminder_ballou_host', SMS_REMINDER_BALLOU_HOST),
		'#required' => TRUE,
		'#size' => 40,
		'#maxlength' => 255,
		'#description' => t('The SMS service host.'),
	);
	
	// Port
	$form['ballou']['sms_reminder_ballou_port'] = array(
		'#type' => 'textfield',
		'#title' => t('Port'),
		'#default_value' => variable_get('sms_reminder_ballou_port', SMS_REMINDER_BALLOU_PORT),
		'#required' => TRUE,
		'#size' => 40,
		'#maxlength' => 255,
		'#description' => t('The port to use.'),
	);
	
	// Username
	$form['ballou']['sms_reminder_ballou_username'] = array(
		'#type' => 'textfield',
		'#title' => t('Username'),
		'#default_value' => variable_get('sms_reminder_ballou_username', NULL),
		'#required' => TRUE,
		'#size' => 40,
		'#maxlength' => 255,
	);
	
	// Password
	$form['ballou']['sms_reminder_ballou_password'] = array(
		'#type' => 'textfield',
		'#title' => t('Password'),
		'#default_value' => variable_get('sms_reminder_ballou_password', NULL),
		'#required' => TRUE,
		'#size' => 40,
		'#maxlength' => 255,
	);
	
	// Sender name
	$form['ballou']['sms_reminder_ballou_from'] = array(
		'#type' => 'textfield',
		'#title' => t('Sender name'),
		'#default_value' => variable_get('sms_reminder_ballou_from', NULL),
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
function sms_reminder_ballou_validate($form, &$form_state) {
}

/**
 * Provider specific submit.
 * @return string Empty string on success or error message on failure.
 */
function sms_reminder_ballou_submit($form, &$form_state) {
	// Get reminder settings
	$ids = explode(',', $form_state['values']['sms_reminder']);
	$reminder = _sms_reminder_get_record($ids[0], $ids[1]);
	
	// Calculate send time
	$event_time = $reminder['event_time'];
	
	$date = array(
		'hour' => date('H', $event_time),
		'minute' => date('i', $event_time),
		'second' => 0,
		'month' => date('m', $event_time),
		'day' => date('d', $event_time),
		'year' => date('Y', $event_time),
	);
	
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
	$username = variable_get('sms_reminder_ballou_username', NULL);
	$password = variable_get('sms_reminder_ballou_password', NULL);
	$from = variable_get('sms_reminder_ballou_from', variable_get('site_name', 'Drupal'));
	
	// UTF-8 decode
	$to = utf8_decode($form_state['values']['phone']);
	$message = utf8_decode($reminder['message']);
	
	// Assemble request document
	$xml = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>";
	$xml .= "<ballou_sms_request>\n";
  $xml .= "<auth username=\"{$username}\" password=\"{$password}\" />\n";
	$xml .= "<request type=\"sms\" from=\"{$from}\" to_msisdn=\"{$to}\"";
	$xml .= !is_null($send_time) ? " date=\"{$send_time}\"" : '';
	$xml .= ">{$message}</request>\n";
	$xml .= "</ballou_sms_request>\n";
	
	// Get host
	$host = variable_get('sms_reminder_ballou_host', SMS_REMINDER_BALLOU_HOST);
	$port = variable_get('sms_reminder_ballou_port', SMS_REMINDER_BALLOU_PORT);
	
	// Put request
	$fp = fsockopen($host, $port, $errno, $errstr, 30);
	if(!$fp) {
		watchdog('sms_reminder', 'Failed sending message using Ballou provider. @errstr (@errno).', array('@errstr' => $errstr, '@errno' => $errno), WATCHDOG_WARNING);
		return false;
	}
	else {
		fputs($fp, "$xml\n");
		while(!feof($fp)) {
			$response .= fgets($fp, 128);
		}
		fclose($fp);
	}
	
	$ret = print_r($response, TRUE);
	
	// Parse response
	$doc = simplexml_load_string($response);
	$errcode = $doc->response->message['error'];
	
	switch($errcode) {
		case 0 :
			$ret = '';
			break;
		case 1 :
			$ret = t('Could not parse request.');
			break;
		case 2 :
			$ret = t('Authentication failed or remote IP rejected.');
			break;
		case 3 :
			$ret = t('Invalid receiver.');
			break;
	}
	
	if(strlen($ret) > 0 ) {
		watchdog('sms_reminder', $ret, NULL, WATCHDOG_ERROR);
	}
	
	return $ret;
}