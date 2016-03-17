<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
namespace OMV\Config;

class DataModel extends \OMV\DataModel\DataModel {
	private $propertiesSchema = null;

	public function __construct($model) {
		// Validate the data model definition.
		$schema = new \OMV\Json\Schema('{
			"type":"object",
			"properties":{
				"type":{"type":"string","enum":["config"],"required":true},
				"id":{"type":"string","required":true},
				"alias":{"type":"string","required":false},
				"title":{"type":"string","required":false},
				"description":{"type":"string","required":false},
				"properties":{"type":"any","required":true},
				"dbbackend":{
					"type":"object",
					"required":false,
					"properties":{
						"xpath":{"type":"string","required":true},
						"iterable":{"type":"boolean","required":true},
						"idproperty":{"type":"string","required":false},
						"refproperty":{"type":"string","required":false}
					}
				}
			}
		}');
		try {
			// Convert the associative array into a JSON string
			// before validating the model definition.
			$schema->validate(json_encode_safe($model));
		} catch(\OMV\Json\SchemaValidationException $e) {
			throw new \OMV\Exception(
			  "The configuration object data model definition is invalid: %s",
			  $e->getMessage());
		}
		// Call the parent constructor.
		parent::__construct($model);
		// Create the schema describing the RPC parameters.
		$this->propertiesSchema = new \OMV\DataModel\Schema(array(
			"type" => "object",
			"properties" => $model['properties']
		));
	}

	public function getPropertiesSchema() {
		return $this->propertiesSchema;
	}

	public function getDbBackendInfo() {
		$model = $this->getModel();
		return $model['dbbackend'];
	}

	public function isIdentifiable() {
		$config = $this->getDbBackendInfo();
		return array_key_exists("idproperty", $config);
	}

	public function getIdProperty() {
		$config = $this->getDbBackendInfo();
		return $config['idproperty'];
	}

	public function isIterable() {
		$config = $this->getDbBackendInfo();
		if (!array_key_exists("iterable", $config))
			return FALSE;
		return $config['iterable'];
	}

	public function isReferenceable() {
		$config = $this->getDbBackendInfo();
		return (array_key_exists("idproperty", $config) && array_key_exists(
		  "refproperty", $config));
	}

	public function getRefProperty() {
		$config = $this->getDbBackendInfo();
		return $config['refproperty'];
	}

	/**
	 * Validate the specified data against the data model.
	 * @param data The JSON data to validate.
	 * @return None.
	 * @throw \OMV\Json\SchemaException
	 * @throw \OMV\Json\SchemaValidationException
	 */
	public function validate($data) {
		$this->getPropertiesSchema()->validate($data);
	}

	public function validateProperty($path, $value) {
		$this->getPropertiesSchema()->validate($value, $path);
	}

	/**
	 * Add a new property to the model.
	 * @param path The path of the property, e.g. "aaa.bbb.ccc".
	 * @param type The type of the property, e.g. 'string' or 'boolean'.
	 */
	public function addProperty($path, $type, array $schema = array()) {
		$schema['type'] = $type;
		$schema = $this->getPropertiesSchema()->addProperty($path, $schema);
	}

	/**
	 * Add a property from the model.
	 * @param path The path of the property, e.g. "aaa.bbb.ccc".
	 */
	public function removeProperty($path) {
		$this->getPropertiesSchema()->removeProperty($path);
	}

	/**
	 * Convert the given value into the type which is declared in the
	 * property schema at the specified path. The original value is returned
	 * when the property type can not be processed, e.g. 'any', 'array',
	 * 'object' or 'null'.
	 * @param name The path of the property, e.g. "aaa.bbb.ccc".
	 * @param value The value to convert.
	 * @return The converted value.
	 */
	public function convertProperty($path, $value) {
		$propSchema = $this->getPropertiesSchema()->getAssocByPath($path);
		if (is_array($propSchema['type'])) {
			throw new \OMV\Json\SchemaException(
			  "%s: The attribute 'type' must not be an array.",
			  $path);
		}
		switch ($propSchema['type']) {
		case "boolean":
			$value = boolvalEx($value);
			break;
		case "integer":
			$value = intval($value);
			break;
		case "number":
		case "double":
		case "float":
			$value = doubleval($value);
			break;
		case "string":
			$value = strval($value);
			break;
		}
		return $value;
	}

	/**
	 * Get the default value of the specified property as defined in
	 * the schema.
	 * @param path The path of the property, e.g. "aaa.bbb.ccc".
	 * @return The default value as specified in the schema or by
	 *   the properties type.
	 */
	public function getPropertyDefault($path) {
		$propSchema = $this->getPropertiesSchema()->getAssocByPath($path);
		$default = NULL;
		if (array_key_exists("default", $propSchema))
			$default = $propSchema['default'];
		else {
			if (is_array($propSchema['type'])) {
				throw new \OMV\Json\SchemaException(
				  "%s: The attribute 'type' must not be an array.",
				  $path);
			}
			$type = "any";
			if (array_key_exists("type", $propSchema))
				$type = $propSchema['type'];
			switch ($type) {
			case "array":
				$default = array(); // Indexed array
				break;
			case "object":
				$default = array(); // Associative array
				break;
			case "boolean":
				$default = FALSE;
				break;
			case "integer":
				$default = 0;
				break;
			case "number":
			case "double":
			case "float":
				$default = 0.0;
				break;
			case "string":
				$default = "";
				break;
			case "any":
			case "null":
				$default = NULL;
				break;
			}
		}
		return $default;
	}

	public function propertyExists($path) {
		$exists = TRUE;
		try {
			$this->getPropertiesSchema()->getAssocByPath($path);
		} catch(\OMV\Json\SchemaException $e) {
			$exists = FALSE;
		} catch(\OMV\Json\SchemaPathException $e) {
			$exists = FALSE;
		}
		return $exists;
	}

	/**
	 * Helper function.
	 */
	private function _walk($name, $path, $schema, $callback, &$userData = NULL) {
//		$this->debug(var_export(func_get_args(), TRUE));
		if (!array_key_exists("type", $schema)) {
			throw new \OMV\Json\SchemaException(
			  "No 'type' attribute defined at '%s'.", $path);
		}
		if ("object" == $schema['type']) {
			if (!array_key_exists("properties", $schema)) {
				throw new \OMV\Json\SchemaException(
				  "No 'properties' attribute defined at '%s'.", $path);
			}
			// Call the callback function. Abort if the callback function
			// returns FALSE.
			if (FALSE === call_user_func_array($callback, array($this, $name,
			  $path, $schema, &$userData)))
				return;
			foreach ($schema['properties'] as $propName => $propSchema) {
				// Build the property path. Take care that a valid path
				// is generated. To ensure this, empty parts are removed.
				$propPath = implode(".", array_filter(array($path, $propName)));
				// Call the callback function. Stop processing this property
				// if the callback function returns FALSE.
				if (FALSE === call_user_func_array($callback, array($this,
				  $propName, $propPath, $propSchema, &$userData)))
					continue;
				// Process the property node.
				$this->_walk($propName, $propPath, $propSchema, $callback,
				  $userData);
			}
		}
	}

	/**
	 * Apply a user function recursively to every property of the schema.
	 * @param path The path of the property, e.g. "aaa.bbb.ccc".
	 * @param callback The callback function.
	 * @param userdata If the optional userdata parameter is supplied, it
	 *   will be passed to the callback function.
	 */
	public function walkRecursive($path, $callback, &$userData = NULL) {
		$propSchema = $this->getPropertiesSchema()->getAssocByPath($path);
		$this->_walk("", $path, $propSchema, $callback, $userData);
	}
}